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
								CefCursorHandle cursor);

	virtual bool StartDragging(CefRefPtr<CefBrowser> browser,
								CefRefPtr<CefDragData> drag_data,
								DragOperationsMask allowed_ops,
								int x, int y);

	virtual void UpdateDragCursor(CefRefPtr<CefBrowser> browser,
								DragOperation operation);


	virtual void OnScrollOffsetChanged(CefRefPtr<CefBrowser> browser);

	virtual void SetCursor( vgui::CursorCode cursor );

	// For texture generation
	unsigned char *GetTextureBuffer() { return m_pTextureBuffer; }
	int GetWidth() { return m_iWidth; }
	int GetHeight() { return m_iHeight; }

	//void LockTextureBuffer() { s_BufferMutex.Lock(); }
	//void UnlockTextureBuffer() { s_BufferMutex.Unlock(); }

	int GetAlphaAt( int x, int y );

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

private:
	int m_iWidth, m_iHeight;
	unsigned char *m_pTextureBuffer;

	//CThreadFastMutex s_BufferMutex;

	SrcCefBrowser *m_pBrowser;

	IMPLEMENT_REFCOUNTING(SrcCefOSRRenderer);
};

#endif // SRC_CEF_RENDERER_H