//====== Copyright © 20xx, Sander Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "src_cef.h"
#include "src_cef_browser.h"
#include "src_cef_osrenderer.h"

#include <vgui/IInput.h>

#include <set>

// CEF
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/cef_runnable.h"
#include "include/cef_client.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

WNDPROC RealWndProc;

LRESULT CALLBACK CefWndProcHook(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

ConVar g_debug_cef("g_debug_cef", "0");

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

LRESULT CALLBACK CefWndProcHook(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_SYSCHAR:
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYUP:
	case WM_CHAR:
		CEFSystem().ProcessKeyInput( message, wParam, lParam );
	default:
		break;
	}
 
	return CallWindowProc(RealWndProc, hWnd, message, wParam, lParam);
}

//-----------------------------------------------------------------------------
// Purpose: Client App
//-----------------------------------------------------------------------------
class ClientApp : public CefApp,
					public CefBrowserProcessHandler
{
public:
	ClientApp();

protected:
	// CefBrowserProcessHandler
	virtual void OnContextInitialized();

private:
	// CefApp methods.
	virtual CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() { return this; }

private:
	IMPLEMENT_REFCOUNTING( ClientApp );

};

CefRefPtr<ClientApp> g_pClientApp;

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
ClientApp::ClientApp()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void ClientApp::OnContextInitialized()
{
	//Msg("ClientApp::OnContextInitialized\n" );
}

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

	RealWndProc = (WNDPROC)SetWindowLong(GetMainWindow(), GWL_WNDPROC, (LONG)CefWndProcHook);

	// Arguments
	HINSTANCE hinst = (HINSTANCE)GetModuleHandle(NULL);
	CefMainArgs main_args( hinst );
	g_pClientApp = new ClientApp;

	// Settings
	CefSettings settings;
	settings.single_process = 0;
	settings.multi_threaded_message_loop = 0;
	settings.log_severity = LOGSEVERITY_ERROR_REPORT;

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
	CefDoMessageLoopWork();

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
	if( m_CefBrowsers[idx]->IsValid() && m_CefBrowsers.IsValidIndex( idx ) )
		return m_CefBrowsers[idx];
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
SrcCefBrowser *CCefSystem::FindBrowser( CefBrowser *pBrowser )
{
	for( int i = 0; i < m_CefBrowsers.Count(); i++ )
	{
		if( m_CefBrowsers[i]->IsValid() && m_CefBrowsers[i]->GetBrowser() == pBrowser )
			return m_CefBrowsers[i];
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CCefSystem::KeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	for( int i = 0; i < m_CefBrowsers.Count(); i++ )
	{
		if( !m_CefBrowsers[i]->IsValid() )
			continue;

		int ret = m_CefBrowsers[i]->KeyInput( down, keynum, pszCurrentBinding );
		if( ret == 0 )
			return 0;
	}
	return 1;
}


static bool isKeyDown(WPARAM wparam) {
  return (GetKeyState(wparam) & 0x8000) != 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static int getKeyModifiers( WPARAM wparam, LPARAM lparam )
{
	int modifiers = 0;
	if (isKeyDown(VK_SHIFT))
		modifiers |= EVENTFLAG_SHIFT_DOWN;
	if (isKeyDown(VK_CONTROL))
		modifiers |= EVENTFLAG_CONTROL_DOWN;
	if (isKeyDown(VK_MENU))
		modifiers |= EVENTFLAG_ALT_DOWN;

	// Low bit set from GetKeyState indicates "toggled".
	if (::GetKeyState(VK_NUMLOCK) & 1)
		modifiers |= EVENTFLAG_NUM_LOCK_ON;
	if (::GetKeyState(VK_CAPITAL) & 1)
		modifiers |= EVENTFLAG_CAPS_LOCK_ON;

	switch (wparam) {
		case VK_RETURN:
			if ((lparam >> 16) & KF_EXTENDED)
				modifiers |= EVENTFLAG_IS_KEY_PAD;
			break;
		case VK_INSERT:
		case VK_DELETE:
		case VK_HOME:
		case VK_END:
		case VK_PRIOR:
		case VK_NEXT:
		case VK_UP:
		case VK_DOWN:
		case VK_LEFT:
		case VK_RIGHT:
			if (!((lparam >> 16) & KF_EXTENDED))
				modifiers |= EVENTFLAG_IS_KEY_PAD;
			break;
		case VK_NUMLOCK:
		case VK_NUMPAD0:
		case VK_NUMPAD1:
		case VK_NUMPAD2:
		case VK_NUMPAD3:
		case VK_NUMPAD4:
		case VK_NUMPAD5:
		case VK_NUMPAD6:
		case VK_NUMPAD7:
		case VK_NUMPAD8:
		case VK_NUMPAD9:
		case VK_DIVIDE:
		case VK_MULTIPLY:
		case VK_SUBTRACT:
		case VK_ADD:
		case VK_DECIMAL:
		case VK_CLEAR:
			modifiers |= EVENTFLAG_IS_KEY_PAD;
			break;
		case VK_SHIFT:
			if (isKeyDown(VK_LSHIFT))
				modifiers |= EVENTFLAG_IS_LEFT;
			else if (isKeyDown(VK_RSHIFT))
				modifiers |= EVENTFLAG_IS_RIGHT;
			break;
		case VK_CONTROL:
			if (isKeyDown(VK_LCONTROL))
				modifiers |= EVENTFLAG_IS_LEFT;
			else if (isKeyDown(VK_RCONTROL))
				modifiers |= EVENTFLAG_IS_RIGHT;
			break;
		case VK_MENU:
			if (isKeyDown(VK_LMENU))
				modifiers |= EVENTFLAG_IS_LEFT;
			else if (isKeyDown(VK_RMENU))
				modifiers |= EVENTFLAG_IS_RIGHT;
			break;
		case VK_LWIN:
			modifiers |= EVENTFLAG_IS_LEFT;
			break;
		case VK_RWIN:
			modifiers |= EVENTFLAG_IS_RIGHT;
		break;
	}
	return modifiers;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCefSystem::ProcessKeyInput( INT message, WPARAM wParam, LPARAM lParam )
{
	if( wParam ==  VK_ESCAPE )
		return;

	CefKeyEvent keyevent;

	keyevent.character = wParam;

	if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN)
		keyevent.type = KEYEVENT_RAWKEYDOWN;
	else if (message == WM_KEYUP || message == WM_SYSKEYUP)
		keyevent.type = KEYEVENT_KEYUP;
	else
		keyevent.type = KEYEVENT_CHAR;

	keyevent.windows_key_code = wParam;
	keyevent.native_key_code = lParam;
	keyevent.is_system_key = message == WM_SYSCHAR ||
							message == WM_SYSKEYDOWN ||
							message == WM_SYSKEYUP;

	keyevent.modifiers = getKeyModifiers( wParam, lParam );

#if 0
	if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN)
		m_LastKeyDownEvent = keyevent;
	else if (message == WM_KEYUP || message == WM_SYSKEYUP)
		m_LastKeyUpEvent = keyevent;
	else	
		m_LastKeyCharEvent = keyevent;
#else
	for( int i = 0; i < m_CefBrowsers.Count(); i++ )
	{
		// Only send key input if no vgui panel has key focus
		// TODO: Deal with game bindings
		vgui::VPANEL focus = vgui::input()->GetFocus();
		vgui::Panel *pPanel = m_CefBrowsers[i]->GetPanel();
		if( !m_CefBrowsers[i]->IsValid() || !m_CefBrowsers[i]->IsGameInputEnabled() || !pPanel->IsVisible() || focus != 0 )
			continue;

		CefRefPtr<CefBrowser> browser = m_CefBrowsers[i]->GetBrowser();

		browser->GetHost()->SendKeyEvent( keyevent );
	}
#endif // 0
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


CON_COMMAND_F( cef_debug_markfulldirty, "", FCVAR_CHEAT )
{
	CUtlVector< SrcCefBrowser * > &browsers = CEFSystem().GetBrowsers();

	for( int i = browsers.Count() - 1; i >= 0; i-- )
	{
		if( browsers[i]->IsValid() )
		{
			int dirtyx, dirtyy, dirtyw, dirtyh;

			// Full dirty
			dirtyx = 0;
			dirtyy = 0;
			dirtyw = browsers[i]->GetOSRHandler()->GetWidth();
			dirtyh = browsers[i]->GetOSRHandler()->GetHeight();

			browsers[i]->GetPanel()->MarkTextureDirty( dirtyx, dirtyy, dirtyw, dirtyh );
		}
	}
}