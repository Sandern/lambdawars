//====== Copyright © 2013 Sandern Corporation, All rights reserved. ===========//
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

// Src CEF
#include "src_cef_browser.h"
#include "src_cef_js.h"
#include "src_cef_vgui_panel.h"

// CEF
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"

extern ConVar g_debug_cef;

//-----------------------------------------------------------------------------
// Purpose: SrcCEF system
//-----------------------------------------------------------------------------
class CCefSystem : public CAutoGameSystemPerFrame
{
public:
	virtual bool Init();
	virtual void Shutdown();
	virtual void Update( float frametime );

	virtual void LevelInitPreEntity();
	virtual void LevelInitPostEntity();

	virtual int KeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

	void ProcessKeyInput( INT message, WPARAM wParam, LPARAM lParam );
	const CefKeyEvent &GetLastKeyUpEvent();
	const CefKeyEvent &GetLastKeyDownEvent();
	const CefKeyEvent &GetLastKeyCharEvent();

	HWND GetMainWindow( void );
	void SetMainWIndow( HWND hWnd );

	void AddBrowser( SrcCefBrowser *pBrowser );
	void RemoveBrowser( SrcCefBrowser *pBrowser );
	int CountBrowsers( void );
	SrcCefBrowser *GetBrowser( int idx );
	SrcCefBrowser *FindBrowser( CefBrowser *pBrowser );

	void OnScreenSizeChanged( int nOldWidth, int nOldHeight );

	CUtlVector< SrcCefBrowser * > &GetBrowsers();

private:
	CefKeyEvent m_LastKeyUpEvent;
	CefKeyEvent m_LastKeyDownEvent;
	CefKeyEvent m_LastKeyCharEvent;

	// Main Window..
	HWND m_MainWindow;

	// Browser
	CUtlVector< SrcCefBrowser * > m_CefBrowsers;
};

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
inline void CCefSystem::SetMainWIndow( HWND hWnd )
{
	m_MainWindow = hWnd;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline const CefKeyEvent &CCefSystem::GetLastKeyUpEvent()
{
	return m_LastKeyUpEvent;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline const CefKeyEvent &CCefSystem::GetLastKeyDownEvent()
{
	return m_LastKeyDownEvent;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline const CefKeyEvent &CCefSystem::GetLastKeyCharEvent()
{
	return m_LastKeyCharEvent;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline CUtlVector< SrcCefBrowser * > &CCefSystem::GetBrowsers()
{
	return m_CefBrowsers;
}

CCefSystem &CEFSystem();

#endif // SRC_CEF_H