//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "src_cef_texgen.h"
#include "src_cef_browser.h"
#include "src_cef_osrenderer.h"
#include "src_cef_vgui_panel.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

static ConVar g_debug_cef_test("g_debug_cef_test", "0" );

CCefTextureGenerator::CCefTextureGenerator( SrcCefBrowser *pBrowser ) : m_pBrowser(pBrowser), m_bIsDirty(true)
{

}

//-----------------------------------------------------------------------------
// Purpose: Texture generator
//-----------------------------------------------------------------------------
void CCefTextureGenerator::RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pSubRect )
{
	if( g_debug_cef_test.GetBool() )
		return;

	// Don't regenerate while loading
	if( engine->IsDrawingLoadingImage() )
	{
		int width, height;
		width = m_pBrowser->GetOSRHandler()->GetWidth();
		height = m_pBrowser->GetOSRHandler()->GetHeight();

		m_pBrowser->GetPanel()->MarkTextureDirty( 0, 0, width, height );
		return;
	}

	if( !pSubRect )
	{
		Warning("CCefTextureGenerator: Regenerating image, but no area specified\n");
		return;
	}

	if( !m_pBrowser->GetOSRHandler() || !m_pBrowser->GetOSRHandler()->GetTextureBuffer() )
		return;

	int width, height, channels;
	int srcwidth, srcheight;
	
	width = pVTFTexture->Width();
	height = pVTFTexture->Height();
	Assert( pVTFTexture->Format() == IMAGE_FORMAT_BGRA8888 );
	channels = 4; 
	
	unsigned char *imageData = pVTFTexture->ImageData( 0, 0, 0 );

	m_bIsDirty = false;

	srcwidth = m_pBrowser->GetOSRHandler()->GetWidth();
	srcheight = m_pBrowser->GetOSRHandler()->GetHeight();

	// Shouldn't happen, but can happen
	if( srcwidth > width || srcheight > height )
		return;

	const unsigned char *srcbuffer = m_pBrowser->GetOSRHandler()->GetTextureBuffer();

	// Copy per row
	int clampedwidth = Min( srcwidth, pSubRect->width );
	int yend = Min( srcheight, pSubRect->y + pSubRect->height );
	int xoffset = (pSubRect->x * channels);
	for( int y = pSubRect->y; y < yend; y++ )
	{
		memcpy( imageData + (y * width * channels) + xoffset, // Destination
			srcbuffer + (y * srcwidth * channels) + xoffset, // Source
			clampedwidth * channels // Row width to copy
		);
	}
}