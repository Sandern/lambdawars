//========= Copyright Sandern Corporation, All rights reserved. ============//
#include "cbase.h"
#include "wars_matchmaking.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#include "clientsteamcontext.h"
#endif // CLIENT_DLL

void WarsRequestGameServer( CSteamID serverSteamId, CSteamID lobbySteamId, KeyValues *pGameData )
{
#ifdef CLIENT_DLL
	if( !steamapicontext )
		return;

	if( !steamapicontext->SteamNetworking() )
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
	V_memcpy( pszData + sizeof(data), keyvaluesData.Base(), keyvaluesData.TellPut());

	//Msg("Wrote game data. Total message size: %d, game data size: %d\n", dataSize, keyvaluesData.TellPut() );
	//KeyValuesDumpAsDevMsg( pGameData, 0, 0 );

	steamapicontext->SteamNetworking()->SendP2PPacket( serverSteamId, pszData, dataSize, k_EP2PSendReliable );
#endif // CLIENT_DLL
}