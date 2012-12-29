//====== Copyright 20xx, Sander Corporation, All rights reserved. =======
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

// Forward declarations
class SrcCefBrowser;

class SrcCefOSRRenderer : public CefRenderHandler 
{
public:
	SrcCefOSRRenderer( SrcCefBrowser *pBrowser, bool transparent );
	virtual ~SrcCefOSRRenderer();

	void Destroy( void ) { m_bValid = false; }

	// ClientHandler::RenderHandler methods
	//virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) OVERRIDE;

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

	// For texture generation
	unsigned char *GetTextureBuffer() { return m_pTextureBuffer; }
	int GetWidth() { return m_iWidth; }
	int GetHeight() { return m_iHeight; }

	void LockTextureBuffer() { s_BufferMutex.Lock(); }
	void UnlockTextureBuffer() { s_BufferMutex.Unlock(); }

	int GetAlphaAt( int x, int y );

private:
	bool m_bValid;
	int m_iWidth, m_iHeight;
	unsigned char *m_pTextureBuffer;

	CThreadFastMutex s_BufferMutex;

	SrcCefBrowser *m_pBrowser;

	IMPLEMENT_REFCOUNTING(SrcCefOSRRenderer);
};

#endif // SRC_CEF_RENDERER_H