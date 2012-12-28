//====== Copyright © 20xx, Sander Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "src_cef.h"
#include "src_cef_browser.h"

// CEF
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/cef_runnable.h"
#include "include/cef_client.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

//WNDPROC RealWndProc;

//LRESULT CALLBACK CefWndProcHook(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

ConVar g_debug_cef("g_debug_cef", "1");

//-----------------------------------------------------------------------------
// Purpose: Helper for finding the main window
//-----------------------------------------------------------------------------
BOOL CALLBACK FindMainWindow(HWND hwnd, LPARAM lParam)
{
	// Filter zero size windows. Don't care about those!
	RECT windowRect;
	GetWindowRect(hwnd, &windowRect);
	if( windowRect.left == 0 && windowRect.top == 0 && windowRect.right == 0 && windowRect.bottom == 0 )
		return true;

	CEFSystem().SetMainWIndow( hwnd );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Client App
//-----------------------------------------------------------------------------
class ClientApp : public CefApp,
                  public CefBrowserProcessHandler,
                  public CefProxyHandler,
                  public CefRenderProcessHandler
{
private:
	IMPLEMENT_REFCOUNTING( ClientApp );

};

CefRefPtr<ClientApp> g_pClientApp;

//-----------------------------------------------------------------------------
// Purpose: SrcCEF system
//-----------------------------------------------------------------------------
bool CCefSystem::Init() 
{
	DevMsg("Initializing CEF\n");

	// Get path to subprocess browser
	char browser_subprocess_path[_MAX_PATH];
	filesystem->RelativePathToFullPath( "bin/cef.exe", "MOD", browser_subprocess_path, _MAX_PATH );

	// Find and set the main window
	EnumThreadWindows( GetCurrentThreadId(), FindMainWindow, (LPARAM) this );

	// Arguments
	HINSTANCE hinst = (HINSTANCE)GetModuleHandle(NULL);
	CefMainArgs main_args( hinst );
	g_pClientApp = new ClientApp;

	// Settings
	CefSettings settings;
	settings.multi_threaded_message_loop = 1;

	CefString(&settings.browser_subprocess_path) = CefString( browser_subprocess_path );

	// Initialize CEF.
	CefInitialize( main_args, settings, g_pClientApp.get() );

	return true; 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCefSystem::Shutdown() 
{
	DevMsg("Shutting down CEF\n");

	// Make sure all browsers are closed
	for( int i = m_CefBrowsers.Count() - 1; i >= 0; i-- )
		m_CefBrowsers[i]->Destroy();

	// Shut down CEF.
	CefShutdown();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCefSystem::Update( float frametime ) 
{ 
	// Perform a single iteration of the CEF message loop
	//CefDoMessageLoopWork();

	// Let browser think
	for( int i = m_CefBrowsers.Count() - 1; i >= 0; i-- )
	{
		if( m_CefBrowsers[i]->IsValid() )
			m_CefBrowsers[i]->Think();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCefSystem::LevelInitPreEntity() 
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCefSystem::LevelInitPostEntity() 
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCefSystem::AddBrowser( SrcCefBrowser *pBrowser )
{
	m_CefBrowsers.AddToTail( pBrowser );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCefSystem::RemoveBrowser( SrcCefBrowser *pBrowser )
{
	m_CefBrowsers.FindAndRemove( pBrowser );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CCefSystem::CountBrowsers( void )
{
	return m_CefBrowsers.Count();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
SrcCefBrowser *CCefSystem::GetBrowser( int idx )
{
	if( m_CefBrowsers.IsValidIndex( idx ) )
		return m_CefBrowsers[idx];
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCefSystem::OnScreenSizeChanged( int nOldWidth, int nOldHeight )
{
	// Invalidate Layout of all browsers. This will make it call PerformLayout next think.
	for( int i = m_CefBrowsers.Count() - 1; i >= 0; i-- )
	{
		if( m_CefBrowsers[i]->IsValid() )
			m_CefBrowsers[i]->InvalidateLayout();
	}
}

static CCefSystem s_CEFSystem;

CCefSystem &CEFSystem()
{
	return s_CEFSystem;
}

#if 1 // _DEBUG
// debug
//-----------------------------------------------------------------------------
// Purpose: Hacky way to detect input focus.
//-----------------------------------------------------------------------------
static int g_debugWinCount;
BOOL CALLBACK DebugEnumerateWindows(HWND hwnd, LPARAM lParam)
{
	RECT windowRect;
	GetWindowRect(hwnd, &windowRect);
	Msg("\t%d Window size: %d %d %d %d\n", hwnd, windowRect.left, windowRect.top, windowRect.right, windowRect.bottom);
	g_debugWinCount++;
	return true;
}

void CC_Debug_Windows_Count( void )
{
	//g_debugWinCount = 0;
	EnumThreadWindows( GetCurrentThreadId(), DebugEnumerateWindows, NULL );
	//EnumWindows( DebugEnumerateWindows, NULL );
	Msg("Window count: %d\n", g_debugWinCount);
	
}
static ConCommand debug_wincount("debug_wincount", CC_Debug_Windows_Count, "", FCVAR_CHEAT);

void CC_Debug_WindowChilds_Count( void )
{
	g_debugWinCount = 0;
	EnumChildWindows( CEFSystem().GetMainWindow(), DebugEnumerateWindows, NULL );
	Msg("Window count childs: %d\n", g_debugWinCount);
	
}
static ConCommand debug_wincount_childs("debug_wincount_childs", CC_Debug_WindowChilds_Count, "", FCVAR_CHEAT);

#endif // _DEBUG