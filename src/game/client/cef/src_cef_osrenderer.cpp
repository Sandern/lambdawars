//====== Copyright © 20xx, Sander Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "src_cef_osrenderer.h"
#include "src_cef_browser.h"
#include "src_cef_vgui_panel.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

SrcCefOSRRenderer::SrcCefOSRRenderer( SrcCefBrowser *pBrowser, bool transparent ) 
	: m_pBrowser(pBrowser), m_pTextureBuffer(NULL), m_iWidth(0), m_iHeight(0)
{

}

SrcCefOSRRenderer::~SrcCefOSRRenderer()
{
	if( m_pTextureBuffer != NULL )
	{
		free( m_pTextureBuffer );
	}
}

bool SrcCefOSRRenderer::GetRootScreenRect(CefRefPtr<CefBrowser> browser,
								CefRect& rect)
{
	rect.x = 0;
	rect.y = 0;
	rect.width = ScreenWidth();
	rect.height = ScreenHeight();
	return true;
}

bool SrcCefOSRRenderer::GetViewRect(CefRefPtr<CefBrowser> browser,
						CefRect& rect)
{
	m_pBrowser->GetPanel()->GetPos( rect.x, rect.y );
	m_pBrowser->GetPanel()->GetSize( rect.width, rect.height );
	return true;
}

bool SrcCefOSRRenderer::GetScreenPoint(CefRefPtr<CefBrowser> browser,
							int viewX,
							int viewY,
							int& screenX,
							int& screenY)
{
	return false;
}

void SrcCefOSRRenderer::OnPopupShow(CefRefPtr<CefBrowser> browser,
						bool show)
{

}

void SrcCefOSRRenderer::OnPopupSize(CefRefPtr<CefBrowser> browser,
						const CefRect& rect)
{

}

void SrcCefOSRRenderer::OnPaint(CefRefPtr<CefBrowser> browser,
					PaintElementType type,
					const RectList& dirtyRects,
					const void* buffer,
					int width,
					int height)
{
	int channels = 4;

	if( type != PET_VIEW )
	{
		Msg("Unsupported paint type\n");
		return;
	}

	// Update image buffer size if needed
	if( m_iWidth != width || m_iHeight != height )
	{
		if( m_pTextureBuffer != NULL )
		{
			free( m_pTextureBuffer );
		}
		Msg("Texture buffer size changed from %dh %dw to %dw %dh\n", m_iWidth, m_iHeight, width, height );
		m_iWidth = width;
		m_iHeight = height;
		m_pTextureBuffer = (unsigned char*) malloc( m_iWidth * m_iHeight * channels );
	}

	const unsigned char *imagebuffer = (const unsigned char *)buffer;
#if 0
	// Update dirty rects
	CefRenderHandler::RectList::const_iterator i = dirtyRects.begin();
	for (; i != dirtyRects.end(); ++i) 
	{
		const CefRect& rect = *i;

		

		//Msg("Copying dirty rect: %d %d %d %d\n", rect.x, rect.y, rect.width, rect.height );
		for( int y = rect.y; y < rect.y + rect.height; y++ )
		{
			memcpy( m_pTextureBuffer + (y * m_iWidth * channels) + (rect.x * channels), // Our current row + x offset
				imagebuffer + (y * m_iWidth * channels) + (rect.x * channels), // Source offset
				rect.width * channels // size of row we want to copy
			); 
		}
	}
#endif // 0
	memcpy( m_pTextureBuffer, imagebuffer, width * height * channels );

	//Msg("SrcCefOSRRenderer::OnPaint\n");
}

void SrcCefOSRRenderer::OnCursorChange(CefRefPtr<CefBrowser> browser,
							CefCursorHandle cursor)
{

}