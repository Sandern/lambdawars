//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef WARS_MATCHMAKING_H
#define WARS_MATCHMAKING_H
#ifdef _WIN32
#pragma once
#endif

#include "steam/steamclientpublic.h"
#include "wars/iwars_extension.h"

enum EGameServerState
{
	k_EGameServer_Error = -1,
	k_EGameServer_Available = 0,
	k_EGameServer_InGame,
	k_EGameServer_InGameFreeStyle, // Not created through matchmaking, but players are on the server
	k_EGameServer_StartingGame, // Delays sending back the "accept game" message until the game server is started
	k_EGameServer_GameEnded, // Tells game server the current game ended
};

typedef struct WarsMessage_t
{
	WarsMessage_t() {}
	WarsMessage_t( uint32 _type ) : type(_type) {}

	//uint16 protocolversion;
	uint32 type;
} WarsMessage_t;

typedef struct WarsRequestServerMessage_t : public WarsMessage_t
{
	CSteamID lobbySteamId;
} WarsRequestServerMessage_t;

typedef struct WarsAcceptGameMessage_t : public WarsMessage_t
{
	WarsAcceptGameMessage_t() {}
	WarsAcceptGameMessage_t( uint32 _type ) : WarsMessage_t(_type) {}

	uint32 publicIP;
	uint32 gamePort;
	uint64 serverSteamID;
} WarsAcceptGameMessage_t;

void WarsRequestGameServer( CSteamID serverSteamId, CSteamID lobbySteamId, KeyValues *pGameData );

#ifdef CLIENT_DLL
void WarsFireMMSessionJoinFailedSignal();
#endif // CLIENT_DLL

#endif // WARS_MATCHMAKING_H