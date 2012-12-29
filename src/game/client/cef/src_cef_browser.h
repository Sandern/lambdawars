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
#include "src_cef_osrenderer.h"

#ifdef WIN32
	#include <winlite.h>
#endif // WIN32

class SrcCefVGUIPanel;

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
	void SetMouseInputEnabled( bool state );
	void SetKeyBoardInputEnabled( bool state );
	bool IsMouseInputEnabled();
	bool IsKeyBoardInputEnabled();
	virtual void SetCursor(vgui::HCursor cursor);
	virtual vgui::HCursor GetCursor();
	void SetUseMouseCapture( bool usemousecapture ) { m_bUseMouseCapture = usemousecapture; }
	bool GetUseMouseCapture( ) { return m_bUseMouseCapture; }

	bool IsAlphaZeroAt( int x, int y );
	int GetAlphaAt( int x, int y );
	void SetPassMouseTruIfAlphaZero( bool passtruifzero ) { m_bPassMouseTruIfAlphaZero = passtruifzero; }
	bool GetPassMouseTruIfAlphaZero( void ) { return m_bPassMouseTruIfAlphaZero; }

	// Notifications
	virtual void OnLoadStart( void ) {}
	virtual void OnLoadEnd( int httpStatusCode ) {}
	virtual void OnLoadError( int errorCode, const wchar_t *errorText, const wchar_t *failedUrl ) {}

	// Browser Methods
	virtual bool IsLoading( void );
	virtual void Reload( void );
	virtual void StopLoad( void );
	virtual void Focus();
	virtual void Unfocus();

	virtual void LoadURL( const char *url );

	virtual void ExecuteJavaScript( const char *code, const char *script_url, int start_line = 0 );

	// Internal
	SrcCefVGUIPanel *GetPanel() { return m_pPanel; }
	CefRefPtr<SrcCefOSRRenderer> GetOSRHandler();
	CefRefPtr<CefBrowser> GetBrowser();

protected:
	CefRefPtr< CefClientHandler > GetClientHandler( void );

private:
	virtual void Think( void );

private:
	CefRefPtr< CefClientHandler > m_CefClientHandler;

	SrcCefVGUIPanel *m_pPanel;

	std::string m_URL;

	bool m_bVisible;
	bool m_bPerformLayout;
	bool m_bPassMouseTruIfAlphaZero;
	bool m_bUseMouseCapture;
};

inline bool SrcCefBrowser::IsAlphaZeroAt( int x, int y )
{
	return GetAlphaAt( x, y ) == 0;
}

#endif // SRC_CEF_BROWSER_H