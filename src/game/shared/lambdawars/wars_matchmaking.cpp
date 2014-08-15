//========= Copyright Sandern Corporation, All rights reserved. ============//
#include "cbase.h"
#include "wars_matchmaking.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#include "clientsteamcontext.h"
#endif // CLIENT_DLL

void WarsSendTestMessage( CSteamID serverSteamId )
{
#ifdef CLIENT_DLL
	if( !steamapicontext )
		return;

	if( !steamapicontext->SteamNetworking() )
		return;

	WarsMessage_t data;
	steamapicontext->SteamNetworking()->SendP2PPacket( serverSteamId, &data, sizeof(data), k_EP2PSendReliable );
#endif // CLIENT_DLL
}