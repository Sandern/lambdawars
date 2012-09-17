//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "hl2wars_hud_chat.h"
#include "hud_macros.h"
#include "text_message.h"
#include "vguicenterprint.h"
#include "hud_basechat.h"
#include <vgui/ILocalize.h>
#include "engine/IEngineSound.h"

#include "c_playerresource.h"

#ifndef DISABLE_PYTHON
	#include "src_python.h"
#endif // DISABLE_PYTHON

#include "c_hl2wars_player.h"
#include "cdll_client_int.h"
#include "ienginevgui.h"

DECLARE_HUDELEMENT( CHudChat );

DECLARE_HUD_MESSAGE( CHudChat, SayText );
DECLARE_HUD_MESSAGE( CHudChat, SayText2 );
DECLARE_HUD_MESSAGE( CHudChat, TextMsg );

//=====================
//CHudChat
//=====================

CHudChat::CHudChat( const char *pElementName ) : BaseClass( pElementName )
{
#if 0
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/SourceScheme.res", "SourceScheme" );
	SetScheme(scheme);

	if ( m_pFiltersButton )
		m_pFiltersButton->SetScheme( scheme );
#endif // 0
}

void CHudChat::Init( void )
{
	BaseClass::Init();

	HOOK_HUD_MESSAGE( CHudChat, SayText );
	HOOK_HUD_MESSAGE( CHudChat, SayText2 );
	HOOK_HUD_MESSAGE( CHudChat, TextMsg );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pszName - 
//			iSize - 
//			*pbuf - 
//-----------------------------------------------------------------------------
void CHudChat::MsgFunc_SayText( bf_read &msg )
{
	char szString[256];

	int client = msg.ReadByte();
	msg.ReadString( szString, sizeof(szString) );
	bool bWantsToChat = msg.ReadByte();

	if ( bWantsToChat )
	{
		// print raw chat text
		ChatPrintf( client, CHAT_FILTER_NONE, "%s", szString );
	}
	else
	{
		// try to lookup translated string
		Printf( CHAT_FILTER_NONE, "%s", hudtextmessage->LookupString( szString ) );
	}

	//CLocalPlayerFilter filter;
	//C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, "HudChat.Message" );

	Msg( "%s", szString );
}

//-----------------------------------------------------------------------------
// Purpose: Reads in a player's Chat text from the server
//-----------------------------------------------------------------------------
void CHudChat::MsgFunc_SayText2( bf_read &msg )
{
	// Got message during connection
	if ( !g_PR )
		return;

	int client = msg.ReadByte();
	bool bWantsToChat = msg.ReadByte();

	wchar_t szBuf[6][256];
	char untranslated_msg_text[256];
	wchar_t *msg_text = ReadLocalizedString( msg, szBuf[0], sizeof( szBuf[0] ), false, untranslated_msg_text, sizeof( untranslated_msg_text ) );

	// keep reading strings and using C format strings for subsituting the strings into the localised text string
	ReadChatTextString ( msg, szBuf[1], sizeof( szBuf[1] ) );		// player name
	ReadChatTextString ( msg, szBuf[2], sizeof( szBuf[2] ) );		// chat text
	ReadLocalizedString( msg, szBuf[3], sizeof( szBuf[3] ), true );
	ReadLocalizedString( msg, szBuf[4], sizeof( szBuf[4] ), true );

	g_pVGuiLocalize->ConstructString( szBuf[5], sizeof( szBuf[5] ), msg_text, 4, szBuf[1], szBuf[2], szBuf[3], szBuf[4] );

	char ansiString[512];
	g_pVGuiLocalize->ConvertUnicodeToANSI( ConvertCRtoNL( szBuf[5] ), ansiString, sizeof( ansiString ) );

	if ( bWantsToChat )
	{
		int iFilter = CHAT_FILTER_NONE;

		if ( client > 0 && (g_PR->GetTeam( client ) != g_PR->GetTeam( GetLocalPlayerIndex() )) )
		{
			iFilter = CHAT_FILTER_PUBLICCHAT;
		}

		// print raw chat text
		ChatPrintf( client, iFilter, "%s", ansiString );

		Msg( "%s\n", RemoveColorMarkup(ansiString) );

		//CLocalPlayerFilter filter;
		//C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, "HudChat.Message" );
	}
	else
	{
		// print raw chat text
		ChatPrintf( client, GetFilterForString( untranslated_msg_text), "%s", ansiString );
	}
}

Color CHudChat::GetClientColor( int clientIndex )
{
	if ( clientIndex == 0 ) // console msg
	{
		return g_ColorYellow;
	}

#ifndef DISABLE_PYTHON
	int ownernumber = g_PR->GetOwnerNumber(clientIndex);

	try	{	
		return boost::python::extract<Color>( SrcPySystem()->Get("_GetColorForOwnerNumber", "playermgr")(ownernumber) );
	} catch(...) {	
		PyErr_Print();	
		PyErr_Clear();	
	}		
#endif // DISABLE_PYTHON
	return g_ColorYellow;
}

Color g_ColorWhite( 255, 255, 255, 255 );
Color CHudChat::GetDefaultTextColor( void )
{
	return g_ColorWhite;
}