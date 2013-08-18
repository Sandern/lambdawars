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

	// CefApp
	virtual void OnBeforeCommandLineProcessing( const CefString& process_type, CefRefPtr<CefCommandLine> command_line );

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

	// CefRenderProcessHandler
	virtual bool OnBeforeNavigation(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  CefRefPtr<CefRequest> request,
                                  NavigationType navigation_type,
                                  bool is_redirect);

	// Message functions
	virtual void SendMsg( CefRefPtr<CefBrowser> browser, const char *pMsg, ... );

	virtual void SendWarning( CefRefPtr<CefBrowser> browser, const char *pMsg, ... );

private:
	std::vector< CefRefPtr<RenderBrowser> > m_Browsers;

	IMPLEMENT_REFCOUNTING( ClientApp );
};

#endif // CLIENT_APP_H