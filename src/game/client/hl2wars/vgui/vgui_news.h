//====== Copyright 20xx, Sander Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef VGUI_NEWS_H
#define VGUI_NEWS_H
#ifdef _WIN32
#pragma once
#endif

#include "src_cef.h"

class WebNews : public SrcCefBrowser
{
public:
	WebNews( vgui::Panel *pParent );

	virtual void OnContextCreated( void );
	virtual void OnLoadEnd( CefRefPtr<CefFrame> frame, int httpStatusCode );
	virtual void PerformLayout( void );
	virtual void OnThink( void );

	virtual void OnMethodCall( int iIdentifier, CefRefPtr<CefListValue> methodargs, int *pCallbackID = NULL );

private:
	vgui::Panel *m_pParent;

	CefString m_CurVersion;

	// Objects and functions
	CefRefPtr<JSObject> m_InterfaceObj;
	CefRefPtr<JSObject> m_LaunchUpdaterFunc;
	CefRefPtr<JSObject> m_OpenURLFunc;
	CefRefPtr<JSObject> m_RetrieveVersionFunc;
	CefRefPtr<JSObject> m_RetrieveDesuraFunc;
};

#endif // VGUI_NEWS_H