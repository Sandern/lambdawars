//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "wars_network.h"
#include "wars_matchmaking.h"

#include "wars/iwars_extension.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

typedef struct WarsEntityUpdateMessage_t : public WarsMessage_t
{
	WarsEntityUpdateMessage_t() {}

	long iEncodedEHandle;
} WarsEntityUpdateMessage_t;

static bool s_EntityUpdateStarted = false;
static WarsEntityUpdateMessage_t s_baseMessageData;
static CUtlBuffer s_variableMessageData;
static CSteamID s_clientSteamID;

void WarsNet_StartEntityUpdate( const CSteamID &steamIDRemote, EHANDLE ent )
{
	if( s_EntityUpdateStarted )
	{
		Warning("WarsNet_StartEntityUpdate: entity update not properly closed\n");
		return;
	}

	//s_pMessageData = warsextension->InsertClientMessage();
	s_EntityUpdateStarted = true;

	// Base information
	s_clientSteamID = steamIDRemote;
	s_variableMessageData.Purge();
	s_baseMessageData.type = k_EMsgClient_PyEntityUpdate;
	int iSerialNum = ent.GetSerialNumber() & (1 << NUM_NETWORKED_EHANDLE_SERIAL_NUMBER_BITS) - 1;
	s_baseMessageData.iEncodedEHandle = ent.GetEntryIndex() | (iSerialNum << MAX_EDICT_BITS);

	s_variableMessageData.Put( &s_baseMessageData, sizeof( s_baseMessageData ) );
}

void WarsNet_EndEntityUpdate()
{
	if( !s_EntityUpdateStarted )
	{
		return;
	}

	s_EntityUpdateStarted = false;

	steamgameserverapicontext->SteamGameServerNetworking()->SendP2PPacket( s_clientSteamID, s_variableMessageData.Base(), s_variableMessageData.Size(), k_EP2PSendReliable );
}

