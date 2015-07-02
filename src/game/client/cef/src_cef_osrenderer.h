//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================

#ifndef SRC_CEF_RENDERER_H
#define SRC_CEF_RENDERER_H
#ifdef _WIN32
#pragma once
#endif

#include "include/cef_render_handler.h"
#include "vgui/Cursor.h"

// Forward declarations
class SrcCefBrowser;

class SrcCefOSRRenderer : public CefRenderHandler 
{
public:
	SrcCefOSRRenderer( SrcCefBrowser *pBrowser, bool transparent );
	virtual ~SrcCefOSRRenderer();

	void Destroy();

	// CefRenderHandler methods
	virtual bool GetRootScreenRect(CefRefPtr<CefBrowser> browser,
									CefRect& rect);
	virtual bool GetViewRect(CefRefPtr<CefBrowser> browser,
							CefRect& rect);
	virtual bool GetScreenPoint(CefRefPtr<CefBrowser> browser,
								int viewX,
								int viewY,
								int& screenX,
								int& screenY);
	virtual void OnPopupShow(CefRefPtr<CefBrowser> browser,
							bool show);
	virtual void OnPopupSize(CefRefPtr<CefBrowser> browser,
							const CefRect& rect);
	virtual void OnPaint(CefRefPtr<CefBrowser> browser,
						PaintElementType type,
						const RectList& dirtyRects,
						const void* buffer,
						int width,
						int height);
	virtual void OnCursorChange(CefRefPtr<CefBrowser> browser,
                              CefCursorHandle cursor,
                              CursorType type,
                              const CefCursorInfo& custom_cursor_info);

	virtual bool StartDragging(CefRefPtr<CefBrowser> browser,
								CefRefPtr<CefDragData> drag_data,
								DragOperationsMask allowed_ops,
								int x, int y);

	virtual void UpdateDragCursor(CefRefPtr<CefBrowser> browser,
								DragOperation operation);


	virtual void OnScrollOffsetChanged(CefRefPtr<CefBrowser> browser);

	virtual void SetCursor( vgui::CursorCode cursor );
	vgui::CursorCode GetCursor();

	// Image buffer containing the pet view
	unsigned char *GetTextureBuffer() { return m_pTextureBuffer; }
	int GetWidth() { return m_iWidth; }
	int GetHeight() { return m_iHeight; }

	// Image buffer containing the popup view (if any)
	unsigned char *GetPopupBuffer() { return m_pPopupBuffer; }
	int GetPopupOffsetX() { return m_iPopupOffsetX; }
	int GetPopupOffsetY() { return m_iPopupOffsetY; }
	int GetPopupWidth() { return m_iPopupWidth; }
	int GetPopupHeight() { return m_iPopupHeight; }

#ifdef USE_MULTITHREADED_MESSAGELOOP
	static CThreadFastMutex &GetTextureBufferMutex() { return s_BufferMutex; }
#endif // USE_MULTITHREADED_MESSAGELOOP

	int GetAlphaAt( int x, int y );

	const CefRect& popup_rect() const { return popup_rect_; }
	const CefRect& original_popup_rect() const { return original_popup_rect_; }

	CefRect GetPopupRectInWebView(const CefRect& original_rect);
	void ClearPopupRects();

	void UpdateRootScreenRect( int x, int y, int wide, int tall );
	void UpdateViewRect( int x, int y, int wide, int tall );

#ifdef WIN32
private:
	// Cursors
	HCURSOR m_hArrow;
	HCURSOR m_hCross;
	HCURSOR m_hHand;
	HCURSOR m_hHelp;
	HCURSOR m_hBeam;
	HCURSOR m_hSizeAll;
	HCURSOR m_hSizeNWSE;
	HCURSOR m_hSizeNESW;
	HCURSOR m_hSizeWE;
	HCURSOR m_hSizeNS;
#endif // WIN32

private:
	bool m_bActive;

	// ===== Game thread data
	SrcCefBrowser *m_pBrowser;

	// ===== CEF UI thread data
	// Texture buffers + cursor can be accessed after locking s_BufferMutex
	int m_iWidth, m_iHeight;
	vgui::CursorCode m_Cursor;
	unsigned char *m_pTextureBuffer;

#ifdef USE_MULTITHREADED_MESSAGELOOP
	static CThreadFastMutex s_BufferMutex;
#endif // USE_MULTITHREADED_MESSAGELOOP

	int m_iPopupOffsetX, m_iPopupOffsetY;
	int m_iPopupWidth, m_iPopupHeight;
	unsigned char *m_pPopupBuffer;

	CefRect popup_rect_;
	CefRect original_popup_rect_;

	CefRect m_rootScreenRect;
	CefRect m_viewRect;

	IMPLEMENT_REFCOUNTING(SrcCefOSRRenderer);
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline vgui::CursorCode SrcCefOSRRenderer::GetCursor()
{
	return m_Cursor;
}

#endif // SRC_CEF_RENDERER_H