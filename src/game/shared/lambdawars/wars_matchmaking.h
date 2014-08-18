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

// Network message types
enum EMessage
{
	// Server messages
	k_EMsgServerRequestGame = 0,

};

typedef struct WarsMessage_t
{
	uint32 type;
} WarsMessage_t;

typedef struct WarsRequestServerMessage_t : public WarsMessage_t
{
	CSteamID lobbySteamId;
} WarsRequestServerMessage_t;

void WarsRequestGameServer( CSteamID serverSteamId, CSteamID lobbySteamId );

#endif // WARS_MATCHMAKING_H