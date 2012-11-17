//====== Copyright 20xx, Sander Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "vgui_news.h"
#include "wars_plat_misc.h"

#include <vgui_controls/Panel.h>
#include "vgui/ISurface.h"
#include "vgui/IScheme.h"
#include <vgui/IVGui.h>
#include "vgui_controls/Controls.h"

#include "src_python.h"

#include "steam/steam_api.h"
#include "steam/isteamfriends.h"

#include "GameUI/GameConsole.h"

#include <Awesomium/WebCore.h>

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

#ifdef ENABLE_AWESOMIUM
WebNews::WebNews( vgui::Panel *pParent ) : WebView( false, pParent )
{
	GetWebView()->set_load_listener( this );
	GetWebView()->set_view_listener( this );
	//GetWebView()->set_process_listener( this );

	LoadFile( "ui/mainmenu.html" );

	SetZPos( -1 );

	SetVisible( false );
	SetMouseInputEnabled( true );
	//SetUseMouseCapture( false );

	if( IsValid() )
	{
		m_CurVersion = CREATE_WEBSTRING( SrcPySystem()->RunT< const char * >( SrcPySystem()->Get("_GetVersion", "srcmgr"), "" ) );

		Awesomium::WebString interfacename = CREATE_WEBSTRING( "interface" );
		Awesomium::JSValue v = GetWebView()->CreateGlobalJavascriptObject( interfacename );
		if( GetWebView()->last_error() == Awesomium::kError_None )
		{
			m_JSInterface = v.ToObject();

			m_LaunchUpdaterMethodName = CREATE_WEBSTRING("launchupdater");
			m_HasDesuraMethodName = CREATE_WEBSTRING("hasdesura");
			m_GetVersionMethodName = CREATE_WEBSTRING("getversion");
			m_OpenURLMethodName = CREATE_WEBSTRING("openurl");
			m_JSInterface.SetCustomMethod( m_LaunchUpdaterMethodName, false );
			m_JSInterface.SetCustomMethod( m_HasDesuraMethodName, true );
			m_JSInterface.SetCustomMethod( m_GetVersionMethodName, true );
			m_JSInterface.SetCustomMethod( m_OpenURLMethodName, false );
		}
	}
}

void WebNews::OnMethodCall(Awesomium::WebView* caller,
						unsigned int remote_object_id, 
						const Awesomium::WebString& method_name,
						const Awesomium::JSArray& args)
{
	if( method_name.Compare( m_LaunchUpdaterMethodName ) == 0 )
	{
		Msg("Launching updater\n");


		if( SrcHasProtocolHandler( "desura" ) )
		{
			SrcShellExecute( "desura://launch/mods/half-life-2-wars" );
			//SrcShellExecute( "desura://install/mods/half-life-2-wars" );
			//SrcShellExecute( "desura://refresh" );
			//SrcShellExecute( "desura://update//mods/half-life-2-wars" );
		}
		else
		{
			SrcShellExecute( "http://www.desura.com/" );
		}

		engine->ClientCmd( "exit" );
	}
	else if( method_name.Compare( m_OpenURLMethodName ) == 0 )
	{
		if( args.size() > 0 )
		{
			Awesomium::WebString url = args[0].ToString();
			char buf[MAX_PATH];
			url.ToUTF8(buf, MAX_PATH);

			steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( buf );
		}
	}
}

Awesomium::JSValue WebNews::OnMethodCallWithReturnValue(Awesomium::WebView* caller,
											unsigned int remote_object_id,
											const Awesomium::WebString& method_name,
											const Awesomium::JSArray& args)
{
	if( method_name.Compare( m_HasDesuraMethodName ) == 0 )
	{
		return Awesomium::JSValue( SrcHasProtocolHandler( "desura" ) );
	}
	else if( method_name.Compare( m_GetVersionMethodName ) == 0 )
	{
		return Awesomium::JSValue( m_CurVersion );
	}

	return Awesomium::JSValue();
}

void WebNews::PerformLayout()
{
	//vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetPanel()->GetScheme() );

	int screenWide, screenTall;
	vgui::surface()->GetScreenSize( screenWide, screenTall );

	int insetx = vgui::scheme()->GetProportionalScaledValue( 200 );
	int insety = vgui::scheme()->GetProportionalScaledValue( 20 );
	SetPos( insetx, insety );
	SetSize( screenWide - insetx - insety, screenTall - insety * 2 );
}

void WebNews::OnDocumentReady(Awesomium::WebView* caller,
							const Awesomium::WebURL& url)
{
	SetVisible( true );
	SetTransparent( true );
}

#elif ENABLE_CEF
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
WebNews::WebNews( vgui::Panel *pParent ) : SrcCefBrowser( "file:///ui/mainmenu.html" ), m_pParent(pParent)
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WebNews::OnAfterCreated( void )
{
	SetVisible( true );
	//SetZPos( -1 );
	//SetMouseInputEnabled( true );
	//SetUseMouseCapture( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WebNews::PerformLayout()
{
	int screenWide, screenTall;
	vgui::surface()->GetScreenSize( screenWide, screenTall );

	int insetx = vgui::scheme()->GetProportionalScaledValue( 200 );
	int insety = vgui::scheme()->GetProportionalScaledValue( 20 );
	SetPos( insetx, insety );
	SetSize( screenWide - insetx - insety, screenTall - insety * 2 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WebNews::OnThink( void )
{
	if( GameConsole().IsConsoleVisible() || !m_pParent->HasFocus() )
	{
		SetVisible( false );
	}
	else
	{
		SetVisible( true );
	}
}

#endif // 0