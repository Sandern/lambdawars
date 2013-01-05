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

#include "src_cef.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
WebNews::WebNews( vgui::Panel *pParent ) : SrcCefBrowser( "MainMenu", "file:///ui/mainmenu.html" ), m_pParent(pParent)
{
	GetPanel()->SetParent( pParent );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WebNews::OnContextCreated()
{
	SetVisible( true );
	SetZPos( -1 );
	SetMouseInputEnabled( true );
	SetUseMouseCapture( false );

	m_CurVersion = SrcPySystem()->RunT< const char * >( SrcPySystem()->Get("_GetVersion", "srcmgr"), "" );

	// Create global interface object
	m_InterfaceObj = CreateGlobalObject("interface");

	// Create and bind functions
	m_LaunchUpdaterFunc = CreateFunction( "launchupdater", m_InterfaceObj );
	m_OpenURLFunc = CreateFunction( "openurl", m_InterfaceObj );

	m_RetrieveVersionFunc = CreateFunction( "retrieveversion", m_InterfaceObj, true );
	m_RetrieveDesuraFunc = CreateFunction( "retrievedesura", m_InterfaceObj, true );
	
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
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WebNews::OnMethodCall( int iIdentifier, CefRefPtr<CefListValue> methodargs, int *pCallbackID )
{
	if( iIdentifier == m_LaunchUpdaterFunc->GetIdentifier() )
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
	else if( iIdentifier == m_OpenURLFunc->GetIdentifier() )
	{
		if( methodargs->GetSize() > 0 )
		{
			CefString url = methodargs->GetString( 0 );

			char buf[MAX_PATH];
			Q_snprintf( buf, MAX_PATH, "%ls", url.c_str() );
			steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( buf );
		}
	}
	else if( iIdentifier == m_RetrieveVersionFunc->GetIdentifier() )
	{
		CefRefPtr<CefListValue> args = CefListValue::Create();
		args->SetString( 0, m_CurVersion );
		SendCallback( pCallbackID, args );
	}
	else if( iIdentifier == m_RetrieveDesuraFunc->GetIdentifier() )
	{
		CefRefPtr<CefListValue> args = CefListValue::Create();
		args->SetBool( 0, SrcHasProtocolHandler( "desura" ) );
		SendCallback( pCallbackID, args );
	}
}