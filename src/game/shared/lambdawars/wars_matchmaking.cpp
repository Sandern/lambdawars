//========= Copyright Sandern Corporation, All rights reserved. ============//
#include "cbase.h"
#include "wars_matchmaking.h"

#ifdef CLIENT_DLL
#include "clientsteamcontext.h"
#include "matchmaking/imatchframework.h"

#ifdef ENABLE_PYTHON
#include "srcpy.h"
#endif // ENABLE_PYTHON
#endif // CLIENT_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
// To prevent handling multiple messages from game servers
static CSteamID s_activeGameServerRequestID;

void WarsRequestGameServer( CSteamID serverSteamId, CSteamID lobbySteamId, KeyValues *pGameData )
{
	if( !steamapicontext )
		return;

	if( !pGameData )
	{
		Warning( "WarsRequestGameServer: no game data specified\n" );
		return;
	}

	bool bIsRequestingLocalGameServer = steamapicontext->SteamUser()->GetSteamID() == serverSteamId;

	CUtlBuffer keyvaluesData;
	pGameData->WriteAsBinary( keyvaluesData );

	WarsRequestServerMessage_t data;
	data.type = bIsRequestingLocalGameServer ? k_EMsgLocalServerRequestGame : k_EMsgServerRequestGame;
	data.lobbySteamId = lobbySteamId;

	int dataSize = sizeof(data) + keyvaluesData.TellPut();
	char* pszData = (char*)stackalloc( dataSize );
	V_memcpy( pszData, &data, sizeof(data) );
	V_memcpy( pszData + sizeof(data), keyvaluesData.Base(), keyvaluesData.TellPut() );

	//Msg("Wrote game data. Total message size: %d, game data size: %d\n", dataSize, keyvaluesData.TellPut() );
	//KeyValuesDumpAsDevMsg( pGameData, 0, 0 );
	
	if( bIsRequestingLocalGameServer )
	{
		// Sends the data correctly
		WarsMessageData_t *pMessageData = warsextension->InsertServerMessage();
		pMessageData->buf.Put( pszData, dataSize );
		pMessageData->steamIDRemote = steamapicontext->SteamUser()->GetSteamID();

		// Creates the session
		s_activeGameServerRequestID = serverSteamId;
		g_pMatchFramework->CreateSession( pGameData );
	}
	else
	{
		if( !steamapicontext->SteamNetworking() )
			return;
		s_activeGameServerRequestID = serverSteamId;
		steamapicontext->SteamNetworking()->SendP2PPacket( serverSteamId, pszData, dataSize, k_EP2PSendReliable, WARSNET_SERVER_CHANNEL );
	}
}

CSteamID WarsGetActiveRequestGameServerSteamID()
{
	return s_activeGameServerRequestID;
}

void WarsFireMMErrorSignal( KeyValues *pEvent )
{
#ifdef ENABLE_PYTHON
	boost::python::dict kwargs;
	kwargs["sender"] = boost::python::object();
	kwargs["event"] = *pEvent;
	boost::python::object signal = SrcPySystem()->Get( "mm_error", "core.signals", true );
	SrcPySystem()->CallSignal( signal, kwargs );
#endif // ENABLE_PYTHON
}

void WarsSendPingMessage( CSteamID steamId )
{
	if( !steamapicontext->SteamNetworking() )
		return;

	WarsMessage_t msgData;
	msgData.type = k_EMsgClient_Ping;

	CUtlBuffer buf;
	buf.Put( &msgData, sizeof( msgData ) );

	steamapicontext->SteamNetworking()->SendP2PPacket( steamId, buf.Base(), buf.TellMaxPut(), k_EP2PSendReliable, WARSNET_CLIENT_CHANNEL );
}
void WarsSendPongMessage( CSteamID steamId )
{
	if( !steamapicontext->SteamNetworking() )
		return;

	WarsMessage_t msgData;
	msgData.type = k_EMsgClient_Pong;

	CUtlBuffer buf;
	buf.Put( &msgData, sizeof( msgData ) );

	steamapicontext->SteamNetworking()->SendP2PPacket( steamId, buf.Base(), buf.TellMaxPut(), k_EP2PSendReliable, WARSNET_CLIENT_CHANNEL );
}
#endif // CLIENT_DLL