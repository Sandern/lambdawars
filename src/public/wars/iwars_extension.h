#ifndef WARS_EXTENSION_INT_H
#define WARS_EXTENSION_INT_H
#ifdef _WIN32
#pragma once
#endif

#include "appframework/IAppSystem.h"
#include "utlvector.h"
#include "utlbuffer.h"
#include "steam/steamclientpublic.h"

class KeyValues;
class ISteamNetworking;

// Network message types
enum EMessage
{
	// Server messages
	k_EMsgServerRequestGame = 0,

	// Client Messages
	k_EMsgClientFirstMsg,
	k_EMsgClientRequestGameAccepted = k_EMsgClientFirstMsg,
	k_EMsgClientRequestGameDenied,
};

typedef struct WarsMessageData_t
{
	CUtlBuffer buf;
	CSteamID steamIDRemote;
} WarsMessageData_t;

//-----------------------------------------------------------------------------
// Purpose: Interface exposed from the wars extension .dll (to the game dlls)
//-----------------------------------------------------------------------------
abstract_class IWarsExtension : public IAppSystem
{
public:
	virtual void ClearData() = 0;

	// Wars Editor functions
	// Methods for syncing commands between server and client through
	// a direct bridge.
	virtual void QueueClientCommand( KeyValues *pCommand ) = 0;
	virtual void QueueServerCommand( KeyValues *pCommand ) = 0;
	virtual KeyValues *PopClientCommandQueue() = 0;
	virtual KeyValues *PopServerCommandQueue() = 0;

	// Receiving Steam P2P messages
	virtual void ReceiveSteamP2PMessages( ISteamNetworking *pSteamNetworking ) = 0;
	virtual WarsMessageData_t *ServerMessageHead() = 0;
	virtual bool NextServerMessage() = 0;
	virtual WarsMessageData_t *ClientMessageHead() = 0;
	virtual bool NextClientMessage() = 0;
};

#define WARS_EXTENSION_VERSION		"VWarsExtension001"

#endif // WARS_EXTENSION_INT_H
