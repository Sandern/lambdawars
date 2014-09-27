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

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

extern ConVar g_debug_cef;
ConVar cef_alpha_force_zero("cef_alpha_force_zero", "0");

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
SrcCefOSRRenderer::SrcCefOSRRenderer( SrcCefBrowser *pBrowser, bool transparent ) 
	: m_pBrowser(pBrowser), m_pTextureBuffer(NULL), m_iWidth(0), m_iHeight(0)
{
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
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
SrcCefOSRRenderer::~SrcCefOSRRenderer()
{
	if( m_pTextureBuffer != NULL )
	{
		free( m_pTextureBuffer );
		m_pTextureBuffer = NULL;
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
	if( !m_pBrowser || !m_pBrowser->GetPanel() )
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
	if( !m_pBrowser || !m_pBrowser->GetPanel() )
		return false;
	screenX = viewX;
	screenY = viewY;
	m_pBrowser->GetPanel()->LocalToScreen( screenX, screenY );
	return true;
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
	if( !m_pBrowser || !m_pBrowser->GetPanel() )
	{
		Warning("SrcCefOSRRenderer::OnPaint: No browser or vgui panel yet\n");
		return;
	}

	int channels = 4;

	if( type != PET_VIEW )
	{
		Warning("SrcCefOSRRenderer::OnPaint: Unsupported paint type\n");
		return;
	}

	Assert( dirtyRects.size() > 0 );

	//AUTO_LOCK( s_BufferMutex );

	// Update image buffer size if needed
	if( m_iWidth != width || m_iHeight != height )
	{
		if( m_pTextureBuffer != NULL )
		{
			free( m_pTextureBuffer );
			m_pTextureBuffer = NULL;
		}
		DevMsg("Texture buffer size changed from %dh %dw to %dw %dh\n", m_iWidth, m_iHeight, width, height );
		m_iWidth = width;
		m_iHeight = height;
		m_pTextureBuffer = (unsigned char*) malloc( m_iWidth * m_iHeight * channels );
	}

	memcpy( m_pTextureBuffer, buffer, m_iWidth * m_iHeight * channels );

	m_pBrowser->GetPanel()->MarkTextureDirty( 0, 0, m_iWidth, m_iHeight );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void SrcCefOSRRenderer::OnCursorChange(CefRefPtr<CefBrowser> browser,
							CefCursorHandle cursor)
{
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
	{
		SetCursor( vgui::dc_arrow );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void SrcCefOSRRenderer::SetCursor( vgui::CursorCode cursor )
{
	CefDbgMsg( 2, "#%dCef: OnCursorChange -> %d\n", m_pBrowser->GetBrowser()->GetIdentifier(), cursor );
	m_pBrowser->GetPanel()->SetCursor( cursor );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int SrcCefOSRRenderer::GetAlphaAt( int x, int y )
{
	if( cef_alpha_force_zero.GetBool() )
		return 0;

	if( x < 0 || y < 0 || x >= m_iWidth || y >= m_iHeight )
		return 0;

	int channels = 4;

	//AUTO_LOCK( s_BufferMutex );
	unsigned char *pImageData = m_pTextureBuffer;
	unsigned char alpha = pImageData ? pImageData[(y * m_iWidth * channels) + (x * channels) + 3] : 0;
	return alpha;
}

bool SrcCefOSRRenderer::StartDragging(CefRefPtr<CefBrowser> browser,
							CefRefPtr<CefDragData> drag_data,
							DragOperationsMask allowed_ops,
							int x, int y)
{
	CefDbgMsg( 2, "#%dCef: SrcCefOSRRenderer StartDragging called\n", m_pBrowser->GetBrowser()->GetIdentifier() );
	if( !browser->GetHost() )
		return false;
	browser->GetHost()->DragSourceSystemDragEnded();
	return true;
}

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