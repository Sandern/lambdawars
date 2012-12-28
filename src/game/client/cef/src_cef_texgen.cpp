//====== Copyright © 20xx, Sander Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "src_cef_texgen.h"
#include "src_cef_browser.h"
#include "src_cef_osrenderer.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

CCefTextureGenerator::CCefTextureGenerator( SrcCefBrowser *pBrowser ) : m_pBrowser(pBrowser), m_bIsDirty(true)
{

}

//-----------------------------------------------------------------------------
// Purpose: Texture generator
//-----------------------------------------------------------------------------
void CCefTextureGenerator::RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pSubRect )
{
	int width, height, channels;
	int srcwidth, srcheight;
	

	width = pVTFTexture->Width();
	height = pVTFTexture->Height();
	Assert( pVTFTexture->Format() == IMAGE_FORMAT_BGRA8888 );
	channels = 4; 
	
	unsigned char *imageData = pVTFTexture->ImageData( 0, 0, 0 );

	if( !m_pBrowser->GetOSRHandler() || !m_pBrowser->GetOSRHandler()->GetTextureBuffer() )
		return;

	m_bIsDirty = false;

	srcwidth = m_pBrowser->GetOSRHandler()->GetWidth();
	srcheight = m_pBrowser->GetOSRHandler()->GetHeight();

	const unsigned char *srcbuffer = m_pBrowser->GetOSRHandler()->GetTextureBuffer();

	// Copy per row
	for( int y = 0; y < srcheight; y++ )
	{
		memcpy( imageData + (y * width * channels), // Destination
			srcbuffer + (y * srcwidth * channels), // Source
			srcwidth * channels // Row width
		);
	}
}