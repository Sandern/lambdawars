//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================

#pragma warning (disable: 4100)
#pragma warning (disable: 4530)

#include "cbase.h"
#include "src_cef_vtf_handler.h"
#include <filesystem.h>

// CefStreamResourceHandler is part of the libcef_dll_wrapper project.
#include "include/wrapper/cef_stream_resource_handler.h"
#include "include/cef_url.h"

#include <jpeglib/jpeglib.h>

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

extern IFileSystem *filesystem;

//-----------------------------------------------------------------------------
// Purpose: Expanded data destination object for CUtlBuffer output
//-----------------------------------------------------------------------------
struct JPEGDestinationManager_t
{
	struct jpeg_destination_mgr pub; // public fields

	CUtlBuffer	*pBuffer; // target/final buffer
	byte		*buffer; // start of temp buffer
};

// choose an efficiently bufferable size
#define OUTPUT_BUF_SIZE  4096	

//-----------------------------------------------------------------------------
// Purpose:  Initialize destination --- called by jpeg_start_compress
//  before any data is actually written.
//-----------------------------------------------------------------------------
METHODDEF(void) init_destination(j_compress_ptr cinfo)
{
	JPEGDestinationManager_t *dest = (JPEGDestinationManager_t *)cinfo->dest;

	// Allocate the output buffer --- it will be released when done with image
	dest->buffer = (byte *)
		(*cinfo->mem->alloc_small) ((j_common_ptr)cinfo, JPOOL_IMAGE,
		OUTPUT_BUF_SIZE * sizeof(byte));

	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}

//-----------------------------------------------------------------------------
// Purpose: Empty the output buffer --- called whenever buffer fills up.
// Input  : boolean - 
//-----------------------------------------------------------------------------
METHODDEF(boolean) empty_output_buffer(j_compress_ptr cinfo)
{
	JPEGDestinationManager_t *dest = (JPEGDestinationManager_t *)cinfo->dest;

	CUtlBuffer *buf = dest->pBuffer;
	buf->Put(dest->buffer, OUTPUT_BUF_SIZE);

	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;

	return TRUE;
}

//-----------------------------------------------------------------------------
// Purpose: Terminate destination --- called by jpeg_finish_compress
// after all data has been written.  Usually needs to flush buffer.
//
// NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
// application must deal with any cleanup that should happen even
// for error exit.
//-----------------------------------------------------------------------------
METHODDEF(void) term_destination(j_compress_ptr cinfo)
{
	JPEGDestinationManager_t *dest = (JPEGDestinationManager_t *)cinfo->dest;
	size_t datacount = OUTPUT_BUF_SIZE - dest->pub.free_in_buffer;

	CUtlBuffer *buf = dest->pBuffer;

	/* Write any data remaining in the buffer */
	if (datacount > 0)
	{
		buf->Put(dest->buffer, datacount);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set up functions for writing data to a CUtlBuffer instead of FILE *
//-----------------------------------------------------------------------------
GLOBAL(void) jpeg_UtlBuffer_dest(j_compress_ptr cinfo, CUtlBuffer *pBuffer)
{
	JPEGDestinationManager_t *dest;

	/* The destination object is made permanent so that multiple JPEG images
	* can be written to the same file without re-executing jpeg_stdio_dest.
	* This makes it dangerous to use this manager and a different destination
	* manager serially with the same JPEG object, because their private object
	* sizes may be different.  Caveat programmer.
	*/
	if (cinfo->dest == NULL) {	/* first time for this JPEG object? */
		cinfo->dest = (struct jpeg_destination_mgr *)
			(*cinfo->mem->alloc_small) ((j_common_ptr)cinfo, JPOOL_PERMANENT,
			sizeof(JPEGDestinationManager_t));
	}

	dest = (JPEGDestinationManager_t *)cinfo->dest;

	dest->pub.init_destination = init_destination;
	dest->pub.empty_output_buffer = empty_output_buffer;
	dest->pub.term_destination = term_destination;
	dest->pBuffer = pBuffer;
}

void VTFHandler_ConvertImageToJPG( CUtlBuffer &buf, unsigned char *imageData, uint32 width, uint32 height )
{
	int quality = 100;

	JSAMPROW row_pointer[1];     // pointer to JSAMPLE row[s]
	int row_stride;              // physical row width in image buffer

	// stderr handler
	struct jpeg_error_mgr jerr;

	// compression data structure
	struct jpeg_compress_struct cinfo;

	row_stride = width * 3; // JSAMPLEs per row in image_buffer

	// point at stderr
	cinfo.err = jpeg_std_error(&jerr);

	// create compressor
	jpeg_create_compress(&cinfo);

	// Hook CUtlBuffer to compression
	jpeg_UtlBuffer_dest(&cinfo, &buf);

	// image width and height, in pixels
	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;

	// Apply settings
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE);

	// Start compressor
	jpeg_start_compress(&cinfo, TRUE);

	// Write scanlines
	while (cinfo.next_scanline < cinfo.image_height)
	{
		row_pointer[0] = &imageData[cinfo.next_scanline * row_stride];
		jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	// Finalize image
	jpeg_finish_compress(&cinfo);

	// Cleanup
	jpeg_destroy_compress(&cinfo);
}

VTFSchemeHandlerFactory::VTFSchemeHandlerFactory()
{
}

CefRefPtr<CefResourceHandler> VTFSchemeHandlerFactory::Create(CefRefPtr<CefBrowser> browser,
											CefRefPtr<CefFrame> frame,
											const CefString& scheme_name,
											CefRefPtr<CefRequest> request)
{
	CefRefPtr<CefResourceHandler> pResourceHandler = NULL;

	CefURLParts parts;
	CefParseURL(request->GetURL(), parts);

	std::string strVtfPath = CefString(&parts.path);

	char vtfPath[MAX_PATH];
	V_snprintf( vtfPath, sizeof( vtfPath ), "materials/%s", strVtfPath.c_str() );
	V_FixupPathName( vtfPath, sizeof( vtfPath ), vtfPath );

	if (!filesystem->FileExists(vtfPath))
	{
		Warning( "VTFSchemeHandlerFactory: invalid vtf %s\n", vtfPath );
		return NULL;
	}

	CUtlBuffer imageDataBuffer( 0, filesystem->Size(vtfPath), 0 );
	if( !filesystem->ReadFile( vtfPath, NULL, imageDataBuffer ) ) 
	{
		Warning( "VTFSchemeHandlerFactory: failed to read vtf %s\n", vtfPath );
		return NULL;
	}

	IVTFTexture *pVTFTexture = CreateVTFTexture();
	if( pVTFTexture->Unserialize( imageDataBuffer ) )
	{
		pVTFTexture->ConvertImageFormat( IMAGE_FORMAT_RGB888, false, false );

		if( pVTFTexture->Format() == IMAGE_FORMAT_RGB888 )
		{
			uint8 *pImageData = pVTFTexture->ImageData();

			CUtlBuffer buf;
			VTFHandler_ConvertImageToJPG( buf, pImageData, pVTFTexture->Width(), pVTFTexture->Height() );

			if( buf.Size() > 0 )
			{
				CefRefPtr<CefStreamReader> stream =
					CefStreamReader::CreateForData(static_cast<void*>(buf.Base()), buf.Size());

				pResourceHandler = new CefStreamResourceHandler("image/jpeg", stream);
			}
		}
		else
		{
			Warning( "VTFSchemeHandlerFactory: unable to convert vtf %s to rgb format\n", vtfPath );
		}
	}
	DestroyVTFTexture( pVTFTexture );

	return pResourceHandler;
}