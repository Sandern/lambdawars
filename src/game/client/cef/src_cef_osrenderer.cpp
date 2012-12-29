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

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
SrcCefOSRRenderer::SrcCefOSRRenderer( SrcCefBrowser *pBrowser, bool transparent ) 
	: m_pBrowser(pBrowser), m_pTextureBuffer(NULL), m_iWidth(0), m_iHeight(0), m_bValid(true)
{

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
SrcCefOSRRenderer::~SrcCefOSRRenderer()
{
	if( m_pTextureBuffer != NULL )
	{
		free( m_pTextureBuffer );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool SrcCefOSRRenderer::GetRootScreenRect(CefRefPtr<CefBrowser> browser,
								CefRect& rect)
{
	rect.x = 0;
	rect.y = 0;
	rect.width = ScreenWidth();
	rect.height = ScreenHeight();
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool SrcCefOSRRenderer::GetViewRect(CefRefPtr<CefBrowser> browser,
						CefRect& rect)
{
	if( !m_bValid )
		return false;
	m_pBrowser->GetPanel()->GetPos( rect.x, rect.y );
	m_pBrowser->GetPanel()->GetSize( rect.width, rect.height );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool SrcCefOSRRenderer::GetScreenPoint(CefRefPtr<CefBrowser> browser,
							int viewX,
							int viewY,
							int& screenX,
							int& screenY)
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void SrcCefOSRRenderer::OnPopupShow(CefRefPtr<CefBrowser> browser,
						bool show)
{

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void SrcCefOSRRenderer::OnPopupSize(CefRefPtr<CefBrowser> browser,
						const CefRect& rect)
{

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void SrcCefOSRRenderer::OnPaint(CefRefPtr<CefBrowser> browser,
					PaintElementType type,
					const RectList& dirtyRects,
					const void* buffer,
					int width,
					int height)
{
	int channels = 4;

	if( !m_bValid )
		return;

	if( type != PET_VIEW )
	{
		Msg("Unsupported paint type\n");
		return;
	}

	s_BufferMutex.Lock();

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

	// Update dirty rects
	CefRenderHandler::RectList::const_iterator i = dirtyRects.begin();
	for (; i != dirtyRects.end(); ++i) 
	{
		const CefRect& rect = *i;

		for( int y = rect.y; y < rect.y + rect.height; y++ )
		{
			memcpy( m_pTextureBuffer + (y * m_iWidth * channels) + (rect.x * channels), // Our current row + x offset
				imagebuffer + (y * m_iWidth * channels) + (rect.x * channels), // Source offset
				rect.width * channels // size of row we want to copy
			); 
		}
	}

	m_pBrowser->GetPanel()->MarkTextureDirty();

	s_BufferMutex.Unlock();

	//DevMsg("SrcCefOSRRenderer::OnPaint. Thread ID: %d\n", GetCurrentThreadId());
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void SrcCefOSRRenderer::OnCursorChange(CefRefPtr<CefBrowser> browser,
							CefCursorHandle cursor)
{

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int SrcCefOSRRenderer::GetAlphaAt( int x, int y )
{
	if( !m_bValid || x < 0 || y < 0 || x >= m_iWidth || y >= m_iHeight )
		return 0;
	unsigned char *pImageData = m_pTextureBuffer;
	if( pImageData )
	{
		int channels = 4;
		return pImageData[(y * m_iWidth * channels) + (x * channels) + 3];
	}
	return 0;
}