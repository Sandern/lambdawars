//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================

#ifndef SRC_CEF_H
#define SRC_CEF_H
#ifdef _WIN32
#pragma once
#endif

#include "igamesystem.h"

#ifdef WIN32
	#include <winlite.h>
	#undef CreateEvent
#endif // WIN32

#ifndef PYPP_GENERATION // FIXME: Generation compiler doesn't likes this...
	// Src CEF
	#include "src_cef_browser.h"
	#include "src_cef_js.h"
	#include "src_cef_vgui_panel.h"

	// CEF
	#include "include/cef_app.h"
	#include "include/cef_browser.h"
	#include "include/cef_frame.h"
#else
	class SrcCefBrowser;
	class CefBrowser;
#endif // PYPP_GENERATION

extern ConVar g_debug_cef;

// Debug
#define CefDbgMsg( lvl, fmt, ... )				\
	if( g_debug_cef.GetInt() >= lvl )			\
		DevMsg( 1, fmt, ##__VA_ARGS__ );		\

//-----------------------------------------------------------------------------
// Purpose: SrcCEF system
//-----------------------------------------------------------------------------
class CCefSystem : public CAutoGameSystemPerFrame
{
public:
	CCefSystem();

	virtual bool Init();
	virtual void Shutdown();
	virtual void Update( float frametime );

	virtual void LevelInitPreEntity();
	virtual void LevelInitPostEntity();

	virtual int KeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

#ifdef WIN32
	void ProcessKeyInput( UINT message, WPARAM wParam, LPARAM lParam );
#endif WIN32

	int GetKeyModifiers();

	void SetFocus( bool focus );

#ifdef WIN32
	HWND GetMainWindow( void );
	void SetMainWindow( HWND hWnd );

	short GetLastMouseWheelDist() { return m_iLastMouseWheelDist; }
	void SetLastMouseWheelDist( short dist ) { m_iLastMouseWheelDist = dist; }
#endif WIN32

	void AddBrowser( SrcCefBrowser *pBrowser );
	void RemoveBrowser( SrcCefBrowser *pBrowser );
	int CountBrowsers( void );
	SrcCefBrowser *GetBrowser( int idx );
	SrcCefBrowser *FindBrowser( CefBrowser *pBrowser );
	SrcCefBrowser *FindBrowserByName( const char *pName );

	void OnScreenSizeChanged( int nOldWidth, int nOldHeight );

	CUtlVector< SrcCefBrowser * > &GetBrowsers();

	bool IsRunning();

private:
	bool m_bIsRunning;
	int m_iKeyModifiers;

	bool m_bHasKeyFocus;

#ifdef WIN32
	// Main Window..
	HWND m_MainWindow;
	// Stores the last mouse movement
	short m_iLastMouseWheelDist;
#endif WIN32

	// Browser
	CUtlVector< SrcCefBrowser * > m_CefBrowsers;
};

#ifdef WIN32
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline HWND CCefSystem::GetMainWindow( void )
{
	return m_MainWindow;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline void CCefSystem::SetMainWindow( HWND hWnd )
{
	m_MainWindow = hWnd;
}
#endif // WIN32

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline int CCefSystem::GetKeyModifiers()
{
	return m_iKeyModifiers;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline void CCefSystem::SetFocus( bool focus )
{
	m_bHasKeyFocus = focus;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline CUtlVector< SrcCefBrowser * > &CCefSystem::GetBrowsers()
{
	return m_CefBrowsers;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline bool CCefSystem::IsRunning()
{
	return m_bIsRunning;
}

CCefSystem &CEFSystem();

#endif // SRC_CEF_H