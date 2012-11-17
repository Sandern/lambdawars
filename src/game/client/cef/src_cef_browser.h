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

class CefClientHandler;

#include "include/internal/cef_ptr.h"

#ifdef WIN32
	#include <winlite.h>
#endif // WIN32

//-----------------------------------------------------------------------------
// Purpose: Cef browser
//-----------------------------------------------------------------------------
class SrcCefBrowser
{
	friend class CCefSystem;

public:
	SrcCefBrowser( const char *pURL = NULL );
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

	// Notifications
	virtual void OnLoadStart( void ) {}
	virtual void OnLoadEnd( int httpStatusCode ) {}
	virtual void OnLoadError( int errorCode, const wchar_t *errorText, const wchar_t *failedUrl ) {}

	// Browser Methods
	virtual bool IsLoading( void );
	virtual void Reload( void );
	virtual void StopLoad( void );

	virtual void LoadURL( const char *url );

	virtual void ExecuteJavaScript( const char *code, const char *script_url, int start_line = 0 );

	// Internal
	HWND		GetWindow( void );

protected:
	CefRefPtr< CefClientHandler > GetClientHandler( void );

private:
	virtual void Think( void );

private:
	CefRefPtr< CefClientHandler > m_CefClientHandler;

	std::string m_URL;

	bool m_bVisible;
	bool m_bPerformLayout;
};

#endif // SRC_CEF_BROWSER_H