//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================

#ifndef SRC_CEF_BROWSER_H
#define SRC_CEF_BROWSER_H
#ifdef _WIN32
#pragma once
#endif

// Indicates to use the mulithreaded message loop
// CefClientHandler will still add individual cases to a message queue, 
// so in SrcCefBrowser they are still processed on the main thread.
#define USE_MULTITHREADED_MESSAGELOOP

#include <string>
#include "src_cef_js.h"
#include "src_cef_vgui_panel.h"
#include "src_cef_osrenderer.h"

#ifndef PYPP_GENERATION // FIXME: Generation compiler doesn't likes this...
// CEF
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/cef_runnable.h"
#include "include/cef_client.h"
#endif // PYPP_GENERATION

class PyJSObject;

#ifndef PYPP_GENERATION // FIXME: Generation compiler doesn't likes this...
//-----------------------------------------------------------------------------
// Purpose: Cef browser internal implementation
//-----------------------------------------------------------------------------
class CefClientHandler : public CefClient,
                      public CefContextMenuHandler,
                      public CefDisplayHandler,
                      public CefDownloadHandler,
					  public CefDragHandler,
                      public CefGeolocationHandler,
                      public CefKeyboardHandler,
                      public CefLifeSpanHandler,
                      public CefLoadHandler,
                      public CefRequestHandler
{
public:
	CefClientHandler( SrcCefBrowser *pSrcBrowser );

	CefRefPtr<CefBrowser> GetBrowser() { return m_Browser; }
	int GetBrowserId() { return m_BrowserId; }

	virtual void Destroy();

	// CefClient methods
	virtual CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() {
		return this;
	}
	virtual CefRefPtr<CefDisplayHandler> GetDisplayHandler() {
		return this;
	}
	virtual CefRefPtr<CefDownloadHandler> GetDownloadHandler() {
		return this;
	}
	virtual CefRefPtr<CefDragHandler> GetDragHandler() {
		return this;
	}
	virtual CefRefPtr<CefGeolocationHandler> GetGeolocationHandler() {
		return this;
	}
	virtual CefRefPtr<CefKeyboardHandler> GetKeyboardHandler() {
		return this;
	}
	virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() {
		return this;
	}
	virtual CefRefPtr<CefLoadHandler> GetLoadHandler() {
		return this;
	}
	virtual CefRefPtr<CefRenderHandler> GetRenderHandler() {
		return m_OSRHandler.get();
	}
	virtual CefRefPtr<CefRequestHandler> GetRequestHandler() {
		return this;
	}

	// Client
	virtual bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
										CefProcessId source_process,
										CefRefPtr<CefProcessMessage> message);

	// CefContextMenuHandler methods
	virtual void OnBeforeContextMenu(CefRefPtr<CefBrowser> browser,
									CefRefPtr<CefFrame> frame,
									CefRefPtr<CefContextMenuParams> params,
									CefRefPtr<CefMenuModel> model);

	// CefDisplayHandler methods
	virtual bool OnConsoleMessage(CefRefPtr<CefBrowser> browser,
								const CefString& message,
								const CefString& source,
								int line);

	// CefDownloadHandler methods
	virtual void OnBeforeDownload(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefDownloadItem> download_item,
		const CefString& suggested_name,
		CefRefPtr<CefBeforeDownloadCallback> callback) {}

	// CefDragHandler methods
	virtual bool OnDragEnter(CefRefPtr<CefBrowser> browser,
                           CefRefPtr<CefDragData> dragData,
                           CefDragHandler::DragOperationsMask mask) { return true; }

	// CefLifeSpanHandler methods
	virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser);

	virtual bool DoClose(CefRefPtr<CefBrowser> browser);

	virtual void OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser,
										TerminationStatus status);

	// CefLoadHandler methods
	virtual void OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame);
	virtual void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode);
	virtual void OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode,
							const CefString& errorText, const CefString& failedUrl);

	virtual void OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
									bool isLoading,
									bool canGoBack,
									bool canGoForward);

	// CefRequestHandler
	virtual bool OnBeforeBrowse(CefRefPtr<CefBrowser> browser,
								CefRefPtr<CefFrame> frame,
								CefRefPtr<CefRequest> request,
								bool is_redirect);

	virtual void OnResourceRedirect(CefRefPtr<CefBrowser> browser,
								CefRefPtr<CefFrame> frame,
								const CefString& old_url,
								CefString& new_url);

	// CefRenderHandler
	void SetOSRHandler(CefRefPtr<SrcCefOSRRenderer> handler) {
		m_OSRHandler = handler;
	}
	CefRefPtr<SrcCefOSRRenderer> GetOSRHandler() { return m_OSRHandler; }

	// Misc
	bool IsInitialized() { return m_bInitialized; }
	float GetLastPingTime() { return m_fLastPingTime; }

private:
	// The child browser window
	CefRefPtr<CefBrowser> m_Browser;
	CefRefPtr<SrcCefOSRRenderer> m_OSRHandler;

	// The child browser id
	int m_BrowserId;

	// Tests if "OnAfterCreated" is called. This means the browser/webview is "initialized"
	bool m_bInitialized;
	// Last time we received a pong from the browser process
	float m_fLastPingTime;

	// Internal
	SrcCefBrowser *m_pSrcBrowser;

#ifdef USE_MULTITHREADED_MESSAGELOOP
	CThreadFastMutex m_MessageQueueMutex;

	enum messageType_e {
		MT_LOADSTART = 0,
		MT_LOADEND,
		MT_LOADERROR,
		MT_LOADINGSTATECHANGE,
		MT_AFTERCREATED,
		MT_CONTEXTCREATED,
		MT_METHODCALL,
	};
	typedef struct messageData_t {
		messageData_t( messageType_e type, CefRefPtr<CefFrame> frame, CefRefPtr<CefListValue> data ) :
			type(type), frame(frame), data(data) {}
		messageType_e type;
		CefRefPtr<CefFrame> frame;
		CefRefPtr<CefListValue> data;
	} messageData_t;
	CUtlVector< messageData_t > m_messageQueue;

	void AddMessage( messageType_e type, CefRefPtr<CefFrame> frame, CefRefPtr<CefListValue> data );
public:
	void ProcessMessages();
#endif // USE_MULTITHREADED_MESSAGELOOP

private:
	IMPLEMENT_REFCOUNTING( CefClientHandler );
};
#endif // PYPP_GENERATION

//-----------------------------------------------------------------------------
// Purpose: Cef browser
//-----------------------------------------------------------------------------
class SrcCefBrowser
{
	friend class CCefSystem;
	friend class CefClientHandler;

public:
	SrcCefBrowser( const char *name, const char *url = "", int renderframerate = 30, int wide = 0, int tall = 0 );
	~SrcCefBrowser();

	void Destroy( void );
	bool IsValid( void );

	// General/Layout
	virtual void OnAfterCreated( void );
	virtual void OnDestroy() {}
	virtual void PerformLayout( void );
	virtual void InvalidateLayout( void );

	// Window Methods
	void SetSize( int wide, int tall );
	void SetPos( int x, int y );
	void SetZPos( int z );
	void SetVisible(bool state);
	bool IsVisible();
	bool IsFullyVisible();
	virtual void OnThink( void ) {}
	void SetMouseInputEnabled( bool state );
	void SetKeyBoardInputEnabled( bool state );
	void SetGameInputEnabled( bool state );
	bool IsMouseInputEnabled();
	bool IsKeyBoardInputEnabled();
	bool IsGameInputEnabled();
	void SetCursor(vgui::HCursor cursor);
	vgui::HCursor GetCursor();
	void SetUseMouseCapture( bool usemousecapture ) { m_bUseMouseCapture = usemousecapture; }
	bool GetUseMouseCapture( ) { return m_bUseMouseCapture; }

	bool IsAlphaZeroAt( int x, int y );
	int GetAlphaAt( int x, int y );
	void SetPassMouseTruIfAlphaZero( bool passtruifzero ) { m_bPassMouseTruIfAlphaZero = passtruifzero; }
	bool GetPassMouseTruIfAlphaZero( void ) { return m_bPassMouseTruIfAlphaZero; }

	virtual int KeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

	void ShowDevTools();
	void CloseDevTools();

	// Load handler methods
	virtual void OnLoadStart( CefRefPtr<CefFrame> frame );
	virtual void OnLoadEnd( CefRefPtr<CefFrame> frame, int httpStatusCode );
	virtual void OnLoadError( CefRefPtr<CefFrame> frame, int errorCode, const wchar_t *errorText, const wchar_t *failedUrl );
	virtual void OnLoadingStateChange( bool isLoading, bool canGoBack, bool canGoForward ) {}

	virtual void OnContextCreated() {}

	// Browser Methods
	virtual bool IsLoading( void );
	virtual void Reload( void );
	virtual void ReloadIgnoreCache( void );
	virtual void StopLoad( void );
	virtual void Focus();
	virtual void Unfocus();

	virtual void LoadURL( const char *url );
	virtual const char *GetURL();

	virtual void WasHidden( bool hidden );

	// Navigation behavior
	enum NavigationType
	{
		NT_DEFAULT = 0, // All navigation is allowed
		NT_PREVENTALL, // Prevent navigating away from the current page
		NT_ONLYFILEPROT, // Only allow navigating to file protocol urls
	};
	virtual void SetNavigationBehavior( NavigationType behavior );
	virtual NavigationType GetNavigationBehavior();

	// Javascript methods
	void ExecuteJavaScript( const char *code, const char *script_url, int start_line = 0 );
	CefRefPtr<JSObject>  ExecuteJavaScriptWithResult( const char *code, const char *script_url, int start_line = 0 );

	CefRefPtr<JSObject> CreateGlobalObject( const char *name );
	CefRefPtr<JSObject> CreateFunction( const char *name, CefRefPtr<JSObject> object = NULL, bool bHasCallback = false );

	void SendCallback( int *pCallbackID, CefRefPtr<CefListValue> methodargs );
	void Invoke( CefRefPtr<JSObject> object, const char *methodname,  CefRefPtr<CefListValue> methodargs );
	CefRefPtr<JSObject> InvokeWithResult( CefRefPtr<JSObject> object, const char *methodname,  CefRefPtr<CefListValue> methodargs );

	// Method Handlers
	virtual void OnMethodCall( CefString identifier, CefRefPtr<CefListValue> methodargs, int *pCallbackID = NULL );

	// Python methods
#ifdef ENABLE_PYTHON
	virtual void PyOnLoadStart( boost::python::object frame ) {}
	virtual void PyOnLoadEnd( boost::python::object frame, int httpStatusCode ) {}
	virtual void PyOnLoadError( boost::python::object frame, int errorCode, boost::python::object errorText, boost::python::object failedUrl ) {}

	boost::python::object PyGetMainFrame();

	boost::python::object PyCreateGlobalObject( const char *name );
	boost::python::object PyCreateFunction( const char *name, PyJSObject *object = NULL, bool hascallback = false );

	boost::python::object PyExecuteJavaScriptWithResult( const char *code, const char *script_url, int start_line = 0 );
	void PySendCallback( boost::python::object callbackid, boost::python::list methodargs );
	void PyInvoke( PyJSObject *object, const char *methodname, boost::python::list methodargs = boost::python::list() );
	boost::python::object PyInvokeWithResult( PyJSObject *object, const char *methodname, boost::python::list methodargs );

	void PyObjectSetAttr( PyJSObject *object, const char *attrname, boost::python::object value );
	boost::python::object PyObjectGetAttr( PyJSObject *object, const char *attrname );

	virtual void PyOnMethodCall( boost::python::object identifier, boost::python::object methodargs, boost::python::object callbackid ) {}
#endif // ENABLE_PYTHON

	void Ping();

	// Internal
	SrcCefVGUIPanel *GetPanel() { return m_pPanel; }
	vgui::VPANEL GetVPanel();
	CefRefPtr<SrcCefOSRRenderer> GetOSRHandler();
	CefRefPtr<CefBrowser> GetBrowser();
	const char *GetName();

protected:
	SrcCefBrowser( SrcCefBrowser const& vec ) { Assert(0); }

private:
	virtual void Think( void );

private:
#ifndef PYPP_GENERATION // FIXME: Generation compiler doesn't likes this...
	CefRefPtr<CefClientHandler> m_CefClientHandler;
#endif // PYPP_GENERATION

	SrcCefVGUIPanel *m_pPanel;

	std::string m_Name;
	std::string m_URL;

	bool m_bVisible;
	bool m_bWasHidden;
	bool m_bPerformLayout;
	bool m_bPassMouseTruIfAlphaZero;
	bool m_bUseMouseCapture;
	bool m_bGameInputEnabled;

	bool m_bHasFocus;
	NavigationType m_navigationBehavior;

	float m_fBrowserCreateTime;
	bool m_bInitializePingSuccessful;
};

inline void SrcCefBrowser::SetGameInputEnabled( bool state )
{
	m_bGameInputEnabled = state;
}

inline bool SrcCefBrowser::IsGameInputEnabled()
{
	return m_bGameInputEnabled;
}

inline const char *SrcCefBrowser::GetName()
{
	return m_Name.c_str();
}

inline bool SrcCefBrowser::IsAlphaZeroAt( int x, int y )
{
	return GetAlphaAt( x, y ) == 0;
}

inline SrcCefBrowser::NavigationType SrcCefBrowser::GetNavigationBehavior()
{
	return m_navigationBehavior;
}

#endif // SRC_CEF_BROWSER_H