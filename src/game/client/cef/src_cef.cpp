//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "src_cef.h"
#include "src_cef_browser.h"
#include "src_cef_osrenderer.h"
#include "src_cef_avatar_handler.h"
#include "src_cef_vtf_handler.h"
#include "src_cef_local_handler.h"
#include "warscef/wars_cef_shared.h"
#include <filesystem.h>
#include "gameui/gameui_interface.h"

#include <vgui/IInput.h>
#include "inputsystem/iinputsystem.h"

#include <set>

#ifdef ENABLE_PYTHON
#include "srcpy.h"
#endif // ENABLE_PYTHON

// CEF
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/cef_runnable.h"
#include "include/cef_client.h"
#include "include/cef_sandbox_win.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

ConVar g_debug_cef("g_debug_cef", "0");
extern ConVar developer;

#if CEF_ENABLE_SANDBOX
// The cef_sandbox.lib static library is currently built with VS2010. It may not
// link successfully with other VS versions.
#pragma comment(lib, "cef_sandbox.lib")
#endif

#ifdef WIN32
static WNDPROC s_pChainedWndProc;

LRESULT CALLBACK CefWndProcHook(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

#if 0
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

	CEFSystem().SetMainWindow( hwnd );
	return true;
}
#endif // 0

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
	{
		CEFSystem().ProcessKeyInput( message, wParam, lParam );
		break;
	}
	case WM_MOUSEWHEEL:
	{
		CEFSystem().SetLastMouseWheelDist( (short)HIWORD(wParam) );
		break;
	}
#ifdef ENABLE_PYTHON
	case WM_INPUTLANGCHANGE:
	{
		SrcPySystem()->CallSignalNoArgs( SrcPySystem()->Get( "gameui_inputlanguage_changed", "core.signals", true ) );
		break;
	}
#endif // ENABLE_PYTHON
	default:
		break;
	}
 
	return CallWindowProc(s_pChainedWndProc, hWnd, message, wParam, lParam);
}
#endif // WIN32

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

	virtual void OnBeforeCommandLineProcessing( const CefString& process_type, CefRefPtr<CefCommandLine> command_line );

	virtual void OnRegisterCustomSchemes( CefRefPtr<CefSchemeRegistrar> registrar);

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
	CefRegisterSchemeHandlerFactory( "steam", "avatarsmall", new AvatarSchemeHandlerFactory( AvatarSchemeHandlerFactory::k_AvatarTypeSmall ) );
	CefRegisterSchemeHandlerFactory( "steam", "avatarmedium", new AvatarSchemeHandlerFactory( AvatarSchemeHandlerFactory::k_AvatarTypeMedium ) );
	CefRegisterSchemeHandlerFactory( "steam", "avatarlarge", new AvatarSchemeHandlerFactory( AvatarSchemeHandlerFactory::k_AvatarTypeLarge ) );
	CefRegisterSchemeHandlerFactory("vtf", "", new VTFSchemeHandlerFactory());
	CefRegisterSchemeHandlerFactory("local", "", new LocalSchemeHandlerFactory());
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void ClientApp::OnBeforeCommandLineProcessing( const CefString& process_type, CefRefPtr<CefCommandLine> command_line )
{
	command_line->AppendSwitch( CefString( "no-proxy-server" ) );

	command_line->AppendSwitch( CefString( "disable-sync" ) );

	if( g_debug_cef.GetBool() )
		DevMsg("Cef Command line arguments: %s\n", command_line->GetCommandLineString().ToString().c_str());
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void ClientApp::OnRegisterCustomSchemes( CefRefPtr<CefSchemeRegistrar> registrar)
{
	registrar->AddCustomScheme("steam", true, false, false);
	registrar->AddCustomScheme("vtf", false /* Not a standard url */, false, false);
	registrar->AddCustomScheme("local", true, false, false);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CCefSystem::CCefSystem() : m_bIsRunning(false), m_bHasKeyFocus(false)
{

}

//-----------------------------------------------------------------------------
// Purpose: SrcCEF system
//-----------------------------------------------------------------------------
bool CCefSystem::Init() 
{
	const bool bEnabled = !CommandLine() || CommandLine()->FindParm("-disablecef") == 0;
	if( !bEnabled )
	{
		Warning("CCefSystem: Not initializing, disabled in command line\n");
		return true;
	}

	const bool bDebugCef = CommandLine() && CommandLine()->FindParm("-debugcef") != 0;
	if( bDebugCef )
		g_debug_cef.SetValue( CommandLine()->ParmValue( "-debugcef", 1 ) );

	// Get path to subprocess browser
	char browser_subprocess_path[_MAX_PATH];
	filesystem->RelativePathToFullPath( "bin/lambdawars_browser.exe", "MOD", browser_subprocess_path, _MAX_PATH );

	// The process sub process file should exist. Error out, because otherwise we can't display the main menu
	if( filesystem->FileExists( browser_subprocess_path ) == false )
	{
		Error( "Could not locate \"%s\". Please check your installation.\n", browser_subprocess_path );
		return false;
	}

#ifdef WIN32 
	// Find and set the main window
	//EnumThreadWindows( GetCurrentThreadId(), FindMainWindow, (LPARAM) this );

	HWND inputWindow = (HWND)inputsystem->GetAttachedWindow();
	//inputsystem->DetachFromWindow();
	SetMainWindow( inputWindow );

	s_pChainedWndProc = reinterpret_cast<WNDPROC>( SetWindowLongPtr( GetMainWindow(), GWL_WNDPROC, reinterpret_cast<LONG_PTR>( CefWndProcHook ) ) );

	//inputsystem->AttachToWindow( inputWindow );
#endif // WIN32

	// Arguments
	HINSTANCE hinst = (HINSTANCE)GetModuleHandle(NULL);
	CefMainArgs main_args( hinst );
	g_pClientApp = new ClientApp;

	// Settings
	CefSettings settings;
	settings.single_process = false;
#ifdef USE_MULTITHREADED_MESSAGELOOP
	settings.multi_threaded_message_loop = true;
#else 
	settings.multi_threaded_message_loop = false;
#endif // USE_MULTITHREADED_MESSAGELOOP
	settings.log_severity = developer.GetBool() ? LOGSEVERITY_VERBOSE : LOGSEVERITY_DEFAULT;
	settings.command_line_args_disabled = true; // Specify args through OnBeforeCommandLineProcessing
	settings.remote_debugging_port = 8088;
	settings.windowless_rendering_enabled = true;
#if !CEF_ENABLE_SANDBOX
	settings.no_sandbox = true;
#endif
	CefString(&settings.cache_path) = CefString( "cache" );

	CefString(&settings.browser_subprocess_path) = CefString( browser_subprocess_path );

	// Initialize CEF.
	void *sandbox_info = NULL;
#if CEF_ENABLE_SANDBOX
	CefScopedSandboxInfo scoped_sandbox;
	sandbox_info = scoped_sandbox.sandbox_info();
#endif

	CefInitialize( main_args, settings, g_pClientApp.get(), sandbox_info );

	DevMsg("Initialized CEF\n");

	m_bIsRunning = true;

	return true; 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCefSystem::Shutdown() 
{
	if( !m_bIsRunning )
		return;

#ifdef ENABLE_PYTHON
	GameUI().ShutdownCEFMenu();
#endif // ENABLE_PYTHON

	DevMsg( "Shutting down CEF (%d active browsers)\n", m_CefBrowsers.Count() );

	// Make sure all browsers are closed
	for( int i = m_CefBrowsers.Count() - 1; i >= 0; i-- )
		m_CefBrowsers[i]->Destroy();
#ifndef USE_MULTITHREADED_MESSAGELOOP
	CefDoMessageLoopWork();
#endif // USE_MULTITHREADED_MESSAGELOOP

	// Shut down CEF.
	CefShutdown();

#ifdef WIN32
	// Restore window process to prevent unwanted callbacks
	SetWindowLongPtr( GetMainWindow(), GWL_WNDPROC, reinterpret_cast<LONG_PTR>( s_pChainedWndProc ) );
#endif // WIN32

	m_bIsRunning = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCefSystem::Update( float frametime ) 
{ 
	if( !m_bIsRunning )
		return;

#ifndef USE_MULTITHREADED_MESSAGELOOP
	// Perform a single iteration of the CEF message loop
	CefDoMessageLoopWork();
#endif // USE_MULTITHREADED_MESSAGELOOP

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
SrcCefBrowser *CCefSystem::FindBrowserByName( const char *pName )
{
	for( int i = 0; i < m_CefBrowsers.Count(); i++ )
	{
		if( m_CefBrowsers[i]->IsValid() && V_strcmp( m_CefBrowsers[i]->GetName(), pName ) == 0 )
			return m_CefBrowsers[i];
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CCefSystem::KeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	// Give browser implementations directly a chance to override the input
	for( int i = 0; i < m_CefBrowsers.Count(); i++ )
	{
		if( !m_CefBrowsers[i]->IsValid() || !m_CefBrowsers[i]->IsFullyVisible() )
			continue;

		int ret = m_CefBrowsers[i]->KeyInput( down, keynum, pszCurrentBinding );
		if( ret == 0 )
			return 0;
	}

	// CEF has key focus, so don't process any keys (apart from the escape key)
	if( m_bHasKeyFocus && ( keynum != KEY_ESCAPE && (!pszCurrentBinding || V_strcmp( "toggleconsole", pszCurrentBinding ) != 0)  ) )
		return 0;

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
void CCefSystem::ProcessKeyInput( UINT message, WPARAM wParam, LPARAM lParam )
{
	if( wParam ==  VK_ESCAPE )
		return;

	CefKeyEvent keyevent;

	if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN)
	{
		keyevent.type = KEYEVENT_RAWKEYDOWN;
	}
	else if (message == WM_KEYUP || message == WM_SYSKEYUP)
	{
		keyevent.type = KEYEVENT_KEYUP;
	}
	else
	{
		keyevent.type = KEYEVENT_CHAR;

		// CEF key event expects the unicode version, but our multi byte application does not
		// receive the right code from the message loop. This is a problem for languages such as
		// Cyrillic. Convert the virtual key to the right unicode char.
		HKL currentKb = ::GetKeyboardLayout( 0 );

		UINT scancode = ( lParam >> 16 ) & 0xFF;
		UINT virtualKey = MapVirtualKeyEx(scancode, MAPVK_VSC_TO_VK, currentKb);

		BYTE kbrdState[256];
		if( GetKeyboardState(kbrdState) )
		{
			wchar_t unicode[2];
			ToUnicodeEx(virtualKey, scancode, (BYTE*)kbrdState, unicode, 2, 0, currentKb);
			//Msg("wParam: %d, Unicode: %d\n", wParam, unicode[0] );

			// Only change wParam if there is a translation for our active keyboard
			if( unicode[0] != 0 )
			{
				wParam = unicode[0];
			}
		}
		/*else
		{
			Warning("Could not get keyboard state\n");
		}*/
	}

	keyevent.character = (wchar_t)wParam;
	keyevent.unmodified_character = (wchar_t)wParam;

	keyevent.windows_key_code = wParam;
	keyevent.native_key_code = lParam;
	keyevent.is_system_key = message == WM_SYSCHAR ||
							message == WM_SYSKEYDOWN ||
							message == WM_SYSKEYUP;

	m_iKeyModifiers = getKeyModifiers( wParam, lParam );
	keyevent.modifiers = m_iKeyModifiers;

	for( int i = 0; i < m_CefBrowsers.Count(); i++ )
	{
		if( !m_CefBrowsers[i]->IsValid() || !m_CefBrowsers[i]->IsFullyVisible() || !m_CefBrowsers[i]->IsGameInputEnabled() )
			continue;

		// Only send key input if no vgui panel has key focus
		// TODO: Deal with game bindings
		vgui::VPANEL focus = vgui::input()->GetFocus();
		vgui::Panel *pPanel = m_CefBrowsers[i]->GetPanel();
		if( !pPanel || !pPanel->IsVisible() || (focus != 0 && focus != pPanel->GetVPanel()) )
			continue;

		CefRefPtr<CefBrowser> browser = m_CefBrowsers[i]->GetBrowser();

		browser->GetHost()->SendKeyEvent( keyevent );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCefSystem::OnScreenSizeChanged( int nOldWidth, int nOldHeight )
{
	// Invalidate Layout of all browsers. This will make it call PerformLayout next think.
	for( int i = m_CefBrowsers.Count() - 1; i >= 0; i-- )
	{
		if( !m_CefBrowsers[i]->IsValid() )
			continue;

		m_CefBrowsers[i]->InvalidateLayout();
		m_CefBrowsers[i]->NotifyScreenInfoChanged();

		m_CefBrowsers[i]->GetPanel()->MarkTextureDirty();
	}
}

static CCefSystem s_CEFSystem;

CCefSystem &CEFSystem()
{
	return s_CEFSystem;
}


CON_COMMAND_F( cef_debug_markfulldirty, "Invalidates render textures of each webview", FCVAR_CHEAT )
{
	CUtlVector< SrcCefBrowser * > &browsers = CEFSystem().GetBrowsers();

	for( int i = browsers.Count() - 1; i >= 0; i-- )
	{
		if( browsers[i]->IsValid() )
		{
			browsers[i]->GetPanel()->MarkTextureDirty();
		}
	}
}

CON_COMMAND_F( cef_debug_printwebviews, "Shows debug information for each webview", FCVAR_CHEAT )
{
	CUtlVector< SrcCefBrowser * > &browsers = CEFSystem().GetBrowsers();

	for( int i = browsers.Count() - 1; i >= 0; i-- )
	{
		if( browsers[i]->IsValid() )
		{
			Msg( "#%d %s:\n", browsers[i]->GetBrowser()->GetIdentifier(), browsers[i]->GetName() );
			Msg( "\tVisible: %d, FullyVisible: %d\n", browsers[i]->GetPanel()->IsVisible(), browsers[i]->GetPanel()->IsFullyVisible() );
		}
	}
}
