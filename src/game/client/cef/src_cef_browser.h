//====== Copyright 20xx, Sander Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef SRC_CEF_BROWSER_H
#define SRC_CEF_BROWSER_H
#ifdef _WIN32
#pragma once
#endif

#include <string>
#include "include/internal/cef_ptr.h"


#ifdef WIN32
	#include <winlite.h>
#endif // WIN32

// Forward declarations
class CefClientHandler;
class SrcCefVGUIPanel;
class SrcCefOSRRenderer;
class CefBrowser;
class CefFrame;
class CefProcessMessage;
class CefListValue;

class JSObject;
class PyJSObject;

//-----------------------------------------------------------------------------
// Purpose: Cef browser
//-----------------------------------------------------------------------------
class SrcCefBrowser
{
	friend class CCefSystem;

public:
	SrcCefBrowser( const char *name, const char *url = "" );
	~SrcCefBrowser();

	void Destroy( void );
	bool IsValid( void );

	// General/Layout
	virtual void OnAfterCreated( void );
	virtual void PerformLayout( void );
	virtual void InvalidateLayout( void );

	// Window Methods
	virtual void SetSize( int wide, int tall );
	virtual void SetPos( int x, int y );
	virtual void SetZPos( int z );
	virtual void SetVisible(bool state);
	virtual bool IsVisible();
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

	// Load handler methods
	virtual void OnLoadStart( CefRefPtr<CefFrame> frame );
	virtual void OnLoadEnd( CefRefPtr<CefFrame> frame, int httpStatusCode );
	virtual void OnLoadError( CefRefPtr<CefFrame> frame, int errorCode, const wchar_t *errorText, const wchar_t *failedUrl );

	virtual void OnContextCreated() {}

	// Browser Methods
	virtual bool IsLoading( void );
	virtual void Reload( void );
	virtual void ReloadIgnoreCache( void );
	virtual void StopLoad( void );
	virtual void Focus();
	virtual void Unfocus();

	virtual void LoadURL( const char *url );

	// Javascript methods
	void ExecuteJavaScript( const char *code, const char *script_url, int start_line = 0 );
	CefRefPtr<JSObject>  ExecuteJavaScriptWithResult( const char *code, const char *script_url, int start_line = 0 );

	CefRefPtr<JSObject> CreateGlobalObject( const char *name );
	CefRefPtr<JSObject> CreateFunction( const char *name, CefRefPtr<JSObject> object = NULL, bool bHasCallback = false );

	void SendCallback( int *pCallbackID, CefRefPtr<CefListValue> methodargs );
	void Invoke( CefRefPtr<JSObject> object, const char *methodname,  CefRefPtr<CefListValue> methodargs );
	CefRefPtr<JSObject> InvokeWithResult( CefRefPtr<JSObject> object, const char *methodname,  CefRefPtr<CefListValue> methodargs );

	// Method Handlers
	virtual void OnMethodCall( int iIdentifier, CefRefPtr<CefListValue> methodargs, int *pCallbackID = NULL );

	// Python methods
#ifdef ENABLE_PYTHON
	virtual void PyOnLoadStart( boost::python::object frame ) {}
	virtual void PyOnLoadEnd( boost::python::object frame, int httpStatusCode ) {}
	virtual void PyOnLoadError( boost::python::object frame, int errorCode, const wchar_t *errorText, const wchar_t *failedUrl ) {}

	boost::python::object PyGetMainFrame();

	boost::python::object PyCreateGlobalObject( const char *name );
	boost::python::object PyCreateFunction( const char *name, PyJSObject *pPyObject = NULL, bool hascallback = false );

	boost::python::object PyExecuteJavaScriptWithResult( const char *code, const char *script_url, int start_line = 0 );
	void PySendCallback( boost::python::object callbackid, boost::python::list methodargs );
	void PyInvoke( PyJSObject *object, const char *methodname, boost::python::list methodargs );
	boost::python::object PyInvokeWithResult( PyJSObject *object, const char *methodname, boost::python::list methodargs );

	virtual void PyOnMethodCall( int identifier, boost::python::object methodargs, boost::python::object callbackid ) {}
#endif // ENABLE_PYTHON

	void Ping();

	// Internal
	SrcCefVGUIPanel *GetPanel() { return m_pPanel; }
	CefRefPtr<SrcCefOSRRenderer> GetOSRHandler();
	CefRefPtr<CefBrowser> GetBrowser();
	const char *GetName();

private:
	virtual void Think( void );

private:
	CefClientHandler *m_CefClientHandler;

	SrcCefVGUIPanel *m_pPanel;

	std::string m_Name;
	std::string m_URL;

	bool m_bVisible;
	bool m_bPerformLayout;
	bool m_bPassMouseTruIfAlphaZero;
	bool m_bUseMouseCapture;
	bool m_bGameInputEnabled;

	bool m_bHasFocus;
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

#endif // SRC_CEF_BROWSER_H