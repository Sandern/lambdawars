//========= Copyright Sandern Corporation, All rights reserved. ============//
#include "cbase.h"
#include "wars_matchmaking.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#include "clientsteamcontext.h"
#endif // CLIENT_DLL

void WarsRequestGameServer( CSteamID serverSteamId, CSteamID lobbySteamId )
{
#ifdef CLIENT_DLL
	if( !steamapicontext )
		return;

	if( !steamapicontext->SteamNetworking() )
		return;

	WarsRequestServerMessage_t data;
	data.type = k_EMsgServerRequestGame;
	data.lobbySteamId = lobbySteamId;
	steamapicontext->SteamNetworking()->SendP2PPacket( serverSteamId, &data, sizeof(data), k_EP2PSendReliable );
#endif // CLIENT_DLL
}