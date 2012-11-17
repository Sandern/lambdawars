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

#ifdef ENABLE_AWESOMIUM
#include "vgui_webview.h"
#undef PostMessage

class WebNews : public WebView
{
public:
	WebNews( vgui::Panel *pParent );

	void OnMethodCall(Awesomium::WebView* caller,
							unsigned int remote_object_id, 
							const Awesomium::WebString& method_name,
							const Awesomium::JSArray& args);

	Awesomium::JSValue OnMethodCallWithReturnValue(Awesomium::WebView* caller,
												unsigned int remote_object_id,
												const Awesomium::WebString& method_name,
												const Awesomium::JSArray& args);

	virtual void PerformLayout();

	virtual void OnDocumentReady(Awesomium::WebView* caller,
								const Awesomium::WebURL& url);

private:
	Awesomium::JSObject m_JSInterface;
	Awesomium::WebString m_CurVersion;

	Awesomium::WebString m_LaunchUpdaterMethodName;
	Awesomium::WebString m_HasDesuraMethodName;
	Awesomium::WebString m_GetVersionMethodName;
	Awesomium::WebString m_OpenURLMethodName;
};
#elif ENABLE_CEF
#include "src_cef_browser.h"

class WebNews : public SrcCefBrowser
{
public:
	WebNews( vgui::Panel *pParent );

	virtual void OnAfterCreated( void );
	virtual void PerformLayout( void );
	virtual void OnThink( void );

private:
	vgui::Panel *m_pParent;
};
#endif // 0

#endif // VGUI_NEWS_H