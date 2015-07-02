//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "src_cef.h"
#include "src_cef_osrenderer.h"
#include "src_cef_browser.h"
#include "src_cef_vgui_panel.h"
#include "vgui/Cursor.h"

// Cef
#include "include/base/cef_bind.h"
#include "include/wrapper/cef_closure_task.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

extern ConVar g_debug_cef;
ConVar cef_alpha_force_zero("cef_alpha_force_zero", "0");
ConVar cef_debug_nopaint("cef_debug_nopaint", "0");

#ifdef USE_MULTITHREADED_MESSAGELOOP
CThreadFastMutex SrcCefOSRRenderer::s_BufferMutex;
#endif // USE_MULTITHREADED_MESSAGELOOP

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
SrcCefOSRRenderer::SrcCefOSRRenderer( SrcCefBrowser *pBrowser, bool transparent ) 
	: m_pBrowser(pBrowser), m_bActive(true), m_pTextureBuffer(NULL), m_pPopupBuffer(NULL), 
	m_iWidth(0), m_iHeight(0), m_iPopupWidth(0), m_iPopupHeight(0)
{
	m_Cursor = vgui::dc_arrow;

	m_rootScreenRect.x = 0;
	m_rootScreenRect.y = 0;
	m_rootScreenRect.width = ScreenWidth();
	m_rootScreenRect.height = ScreenHeight();

	m_viewRect = m_rootScreenRect;

#ifdef WIN32
	m_hArrow = LoadCursor (NULL, IDC_ARROW );
	m_hCross = LoadCursor (NULL, IDC_CROSS );
	m_hHand = LoadCursor (NULL, IDC_HAND );
	m_hHelp = LoadCursor (NULL, IDC_HELP );
	m_hBeam = LoadCursor (NULL, IDC_IBEAM );
	m_hSizeAll = LoadCursor (NULL, IDC_SIZEALL );
	m_hSizeNWSE = LoadCursor (NULL, IDC_SIZENWSE ); // dc_sizenwse
	m_hSizeNESW = LoadCursor (NULL, IDC_SIZENESW ); // dc_sizenesw
	m_hSizeWE = LoadCursor (NULL, IDC_SIZEWE ); // dc_sizewe
	m_hSizeNS = LoadCursor (NULL, IDC_SIZENS ); // dc_sizens
#endif // WIN32
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
SrcCefOSRRenderer::~SrcCefOSRRenderer()
{
	Destroy();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void SrcCefOSRRenderer::Destroy()
{
#ifdef USE_MULTITHREADED_MESSAGELOOP
	AUTO_LOCK( GetTextureBufferMutex() );
#endif // USE_MULTITHREADED_MESSAGELOOP

	m_bActive = false;

	if( m_pTextureBuffer != NULL )
	{
		free( m_pTextureBuffer );
		m_pTextureBuffer = NULL;
	}
	if( m_pPopupBuffer != NULL )
	{
		free( m_pPopupBuffer );
		m_pPopupBuffer = NULL;
	}

	m_pBrowser = NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool SrcCefOSRRenderer::GetRootScreenRect(CefRefPtr<CefBrowser> browser,
								CefRect& rect)
{
#ifndef USE_MULTITHREADED_MESSAGELOOP
	Assert( !CefCurrentlyOn(TID_UI) );
	rect.x = 0;
	rect.y = 0;
	rect.width = ScreenWidth();
	rect.height = ScreenHeight();
#else
	Assert( CefCurrentlyOn(TID_UI) );
	rect = m_rootScreenRect;
#endif // USE_MULTITHREADED_MESSAGELOOP
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool SrcCefOSRRenderer::GetViewRect(CefRefPtr<CefBrowser> browser,
						CefRect& rect)
{
#ifndef USE_MULTITHREADED_MESSAGELOOP
	Assert( !CefCurrentlyOn(TID_UI) );
	if( !m_pBrowser || !m_pBrowser->GetPanel() )
		return false;
	m_pBrowser->GetPanel()->GetPos( rect.x, rect.y );
	m_pBrowser->GetPanel()->GetSize( rect.width, rect.height );
#else
	Assert( CefCurrentlyOn(TID_UI) );
	rect = m_viewRect;
#endif // USE_MULTITHREADED_MESSAGELOOP
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
#ifndef USE_MULTITHREADED_MESSAGELOOP
	Assert( !CefCurrentlyOn(TID_UI) );
	if( !m_pBrowser || !m_pBrowser->GetPanel() )
		return false;
	screenX = viewX;
	screenY = viewY;
	m_pBrowser->GetPanel()->LocalToScreen( screenX, screenY );
#else
	Assert( CefCurrentlyOn(TID_UI) );
	screenX = viewX;
	screenY = viewY;
	// TODO: translate to screen
#endif // USE_MULTITHREADED_MESSAGELOOP
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void SrcCefOSRRenderer::OnPopupShow(CefRefPtr<CefBrowser> browser,
						bool show)
{
	Assert( !CefCurrentlyOn(TID_UI) );
	if (!show) {
#ifdef USE_MULTITHREADED_MESSAGELOOP
		AUTO_LOCK( GetTextureBufferMutex() );
#endif // USE_MULTITHREADED_MESSAGELOOP

		// Clear the popup rectangle.
		ClearPopupRects();

		if( m_pPopupBuffer != NULL )
		{
			free( m_pPopupBuffer );
			m_pPopupBuffer = NULL;
		}

		browser->GetHost()->Invalidate(PET_VIEW);

		m_pBrowser->GetPanel()->MarkTextureDirty();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void SrcCefOSRRenderer::OnPopupSize(CefRefPtr<CefBrowser> browser,
						const CefRect& rect)
{
	Assert( !CefCurrentlyOn(TID_UI) );
	if (rect.width <= 0 || rect.height <= 0)
		return;
	original_popup_rect_ = rect;
	popup_rect_ = GetPopupRectInWebView(original_popup_rect_);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CefRect SrcCefOSRRenderer::GetPopupRectInWebView(const CefRect& original_rect) 
{
	CefRect rc(original_rect);
	// if x or y are negative, move them to 0.
	if (rc.x < 0)
		rc.x = 0;
	if (rc.y < 0)
		rc.y = 0;
	// if popup goes outside the view, try to reposition origin
	if (rc.x + rc.width > m_iWidth)
		rc.x = m_iWidth - rc.width;
	if (rc.y + rc.height > m_iHeight)
		rc.y = m_iHeight - rc.height;
	// if x or y became negative, move them to 0 again.
	if (rc.x < 0)
		rc.x = 0;
	if (rc.y < 0)
		rc.y = 0;
	return rc;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void SrcCefOSRRenderer::ClearPopupRects() 
{
	popup_rect_.Set(0, 0, 0, 0);
	original_popup_rect_.Set(0, 0, 0, 0);
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
#ifdef USE_MULTITHREADED_MESSAGELOOP
	Assert( CefCurrentlyOn(TID_UI) );
	AUTO_LOCK( GetTextureBufferMutex() );
#endif // USE_MULTITHREADED_MESSAGELOOP

	if( !m_bActive || !m_pBrowser || !m_pBrowser->GetPanel() )
	{
		CefDbgMsg( 1, "SrcCefOSRRenderer::OnPaint: No browser or vgui panel yet, or browser is being destroyed.\n" );
		return;
	}

	if( cef_debug_nopaint.GetBool() )
	{
		return;
	}

	const int channels = 4;

	Assert( dirtyRects.size() > 0 );

	if( type == PET_VIEW )
	{
#ifdef RENDER_DIRTY_AREAS
		int dirtyx, dirtyy, dirtyxend, dirtyyend;
		dirtyx = width;
		dirtyy = height;
		dirtyxend = 0;
		dirtyyend = 0;
#endif // 0

		// Update image buffer size if needed
		if( m_iWidth != width || m_iHeight != height )
		{
			if( m_pTextureBuffer != NULL )
			{
				free( m_pTextureBuffer );
				m_pTextureBuffer = NULL;
			}
			CefDbgMsg( 1, "Texture buffer size changed from %dh %dw to %dw %dh\n", m_iWidth, m_iHeight, width, height );
			m_iWidth = width;
			m_iHeight = height;
			m_pTextureBuffer = (unsigned char*) malloc( m_iWidth * m_iHeight * channels );

#ifdef RENDER_DIRTY_AREAS
			// Full dirty
			dirtyx = 0;
			dirtyy = 0;
			dirtyxend = m_iWidth;
			dirtyyend = m_iHeight;
#endif // 0
		}

#ifdef RENDER_DIRTY_AREAS
		const unsigned char *imagebuffer = (const unsigned char *)buffer;

		// Update dirty rects
		CefRenderHandler::RectList::const_iterator i = dirtyRects.begin();
		for (; i != dirtyRects.end(); ++i) 
		{
			const CefRect& rect = *i;

			for( int y = rect.y; y < rect.y + rect.height; y++ )
			{
				V_memcpy( m_pTextureBuffer + (y * m_iWidth * channels) + (rect.x * channels), // Our current row + x offset
					imagebuffer + (y * m_iWidth * channels) + (rect.x * channels), // Source offset
					rect.width * channels // size of row we want to copy
				);
			}

			// Update max dirty area
			dirtyx = Min( rect.x, dirtyx );
			dirtyy = Min( rect.y, dirtyy );
			dirtyxend = Max( rect.x + rect.width, dirtyxend );
			dirtyyend = Max( rect.y + rect.height, dirtyyend );
		}
#else
		// Full frame copy
		const unsigned char *imagebuffer = (const unsigned char *)buffer;
		V_memcpy( m_pTextureBuffer, imagebuffer, m_iWidth * m_iHeight * channels );
#endif // RENDER_DIRTY_AREAS

#ifdef RENDER_DIRTY_AREAS
		m_pBrowser->GetPanel()->MarkTextureDirty( dirtyx, dirtyy, dirtyxend, dirtyyend );
#else
		m_pBrowser->GetPanel()->MarkTextureDirty();
#endif // 0
	}
	else if( type == PET_POPUP )
	{
		// Update image buffer size if needed
		if( !m_pPopupBuffer || m_iPopupWidth != popup_rect_.width || m_iPopupHeight != popup_rect_.height )
		{
			if( m_pPopupBuffer != NULL )
			{
				free( m_pPopupBuffer );
				m_pPopupBuffer = NULL;
			}
			m_iPopupWidth = width;
			m_iPopupHeight = height;
			m_pPopupBuffer = (unsigned char*) malloc( m_iPopupWidth * m_iPopupHeight * channels );
		}

		m_iPopupOffsetX = popup_rect_.x;
		m_iPopupOffsetY = popup_rect_.y;

		V_memcpy( m_pPopupBuffer, buffer, m_iPopupWidth * m_iPopupHeight * channels );

		m_pBrowser->GetPanel()->MarkPopupDirty();
	}
	else
	{
		Warning("SrcCefOSRRenderer::OnPaint: Unsupported paint type %d\n", type);
		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void SrcCefOSRRenderer::OnCursorChange(CefRefPtr<CefBrowser> browser,
                              CefCursorHandle cursor,
                              CursorType type,
                              const CefCursorInfo& custom_cursor_info)
{
	Assert( CefCurrentlyOn(TID_UI) );
#ifdef WIN32
	if( cursor == m_hArrow )
	{
		SetCursor( vgui::dc_arrow );
	}
	else if( cursor == m_hCross )
	{
		SetCursor( vgui::dc_crosshair );
	}
	else if( cursor == m_hHand )
	{
		SetCursor( vgui::dc_hand );
	}
	else if( cursor == m_hHelp )
	{
		SetCursor( vgui::dc_blank );
	}
	else if( cursor == m_hBeam )
	{
		SetCursor( vgui::dc_ibeam );
	}
	else if( cursor == m_hSizeAll )
	{
		SetCursor( vgui::dc_sizeall );
	}
	else if( cursor == m_hSizeNWSE )
	{
		SetCursor( vgui::dc_sizenwse );
	}
	else if( cursor == m_hSizeNESW )
	{
		SetCursor( vgui::dc_sizenesw );
	}
	else if( cursor == m_hSizeWE )
	{
		SetCursor( vgui::dc_sizewe );
	}
	else if( cursor == m_hSizeNS )
	{
		SetCursor( vgui::dc_sizens );
	}
	else
#endif // WIN32
	{
		SetCursor( vgui::dc_arrow );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void SrcCefOSRRenderer::SetCursor( vgui::CursorCode cursor )
{
	CefDbgMsg( 2, "Cef: OnCursorChange -> %d\n", cursor );
	m_Cursor = cursor;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int SrcCefOSRRenderer::GetAlphaAt( int x, int y )
{
	if( cef_alpha_force_zero.GetBool() )
		return 0;

#ifdef USE_MULTITHREADED_MESSAGELOOP
	AUTO_LOCK( GetTextureBufferMutex() );
#endif // USE_MULTITHREADED_MESSAGELOOP

	if( x < 0 || y < 0 || x >= m_iWidth || y >= m_iHeight )
		return 0;

	int channels = 4;

	unsigned char *pImageData = m_pTextureBuffer;
	unsigned char alpha = pImageData ? pImageData[(y * m_iWidth * channels) + (x * channels) + 3] : 0;
	return alpha;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool SrcCefOSRRenderer::StartDragging(CefRefPtr<CefBrowser> browser,
							CefRefPtr<CefDragData> drag_data,
							DragOperationsMask allowed_ops,
							int x, int y)
{
	CefDbgMsg( 2, "#%dCef: SrcCefOSRRenderer StartDragging called\n", m_pBrowser->GetBrowser()->GetIdentifier() );
	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void SrcCefOSRRenderer::UpdateDragCursor(CefRefPtr<CefBrowser> browser,
							DragOperation operation)
{
	CefDbgMsg( 2, "#%dCef: SrcCefOSRRenderer UpdateDragCursor called\n", m_pBrowser->GetBrowser()->GetIdentifier() );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void SrcCefOSRRenderer::OnScrollOffsetChanged(CefRefPtr<CefBrowser> browser)
{
	CefDbgMsg( 2, "#%dCef: SrcCefOSRRenderer OnScrollOffsetChanged called\n", m_pBrowser->GetBrowser()->GetIdentifier() );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void SrcCefOSRRenderer::UpdateRootScreenRect( int x, int y, int wide, int tall )
{
	if (!CefCurrentlyOn(TID_UI)) {
		CefPostTask(TID_UI, 
			base::Bind(&SrcCefOSRRenderer::UpdateRootScreenRect, this,
			x, y, wide, tall));
		return;
	}

	m_rootScreenRect.x = x;
	m_rootScreenRect.y = y;
	m_rootScreenRect.width = wide;
	m_rootScreenRect.height = tall;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void SrcCefOSRRenderer::UpdateViewRect( int x, int y, int wide, int tall )
{
	if (!CefCurrentlyOn(TID_UI)) {
		CefPostTask(TID_UI, 
			base::Bind(&SrcCefOSRRenderer::UpdateViewRect, this,
			x, y, wide, tall));
		return;
	}

	m_viewRect.x = x;
	m_viewRect.y = y;
	m_viewRect.width = wide;
	m_viewRect.height = tall;
}