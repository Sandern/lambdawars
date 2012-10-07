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

#endif // VGUI_NEWS_H