// Copyright (c) 2012 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.


#ifndef CLIENT_APP_H
#define CLIENT_APP_H
#ifdef _WIN32
#pragma once
#endif

#include "include/cef_app.h"
#include "render_browser.h"
#include <vector>

// Forward declarations
class CefBrowser;

// Client app
class ClientApp : public CefApp,
				  public CefRenderProcessHandler
{
public:
	virtual CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() {
		return this;
	}

	// Messages from/to main process
	virtual bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
										CefProcessId source_process,
										CefRefPtr<CefProcessMessage> message);


	// Browser
	virtual void OnBrowserCreated(CefRefPtr<CefBrowser> browser);

	virtual void OnBrowserDestroyed(CefRefPtr<CefBrowser> browser);

	CefRefPtr<RenderBrowser> FindBrowser( CefRefPtr<CefBrowser> browser );

	// Context
	virtual void OnContextCreated(CefRefPtr<CefBrowser> browser,
								CefRefPtr<CefFrame> frame,
								CefRefPtr<CefV8Context> context);

	virtual void OnContextReleased(CefRefPtr<CefBrowser> browser,
									CefRefPtr<CefFrame> frame,
									CefRefPtr<CefV8Context> context);

	// Message functions
	virtual void SendMsg( CefRefPtr<CefBrowser> browser, const char *pMsg, ... );

	virtual void SendWarning( CefRefPtr<CefBrowser> browser, const char *pMsg, ... );

private:
	std::vector< CefRefPtr<RenderBrowser> > m_Browsers;

	IMPLEMENT_REFCOUNTING( ClientApp );
};

#endif // CLIENT_APP_H