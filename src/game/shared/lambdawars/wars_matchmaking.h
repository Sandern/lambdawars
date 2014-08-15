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

// Network message types
enum EMessage
{
	// Server messages
	k_EMsgServerBegin = 0,

};

typedef struct WarsMessage_t
{
	uint32 type;
} WarsMessage_t;

void WarsSendTestMessage( CSteamID serverSteamId );

#endif // WARS_MATCHMAKING_H