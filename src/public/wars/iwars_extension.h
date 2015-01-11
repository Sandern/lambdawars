#ifndef WARS_EXTENSION_INT_H
#define WARS_EXTENSION_INT_H
#ifdef _WIN32
#pragma once
#endif

#include "appframework/IAppSystem.h"
#include "utlvector.h"
#include "utlbuffer.h"
#include "steam/steamclientpublic.h"
#include "inetchannelinfo.h"

class KeyValues;
class ISteamNetworking;

// Source Engine switches to Steam P2P api for sending data
// when behind a firewall, so we can't use the default channel.
// Make sure we are above the used channels.
#define WARSNET_SERVER_CHANNEL INetChannelInfo::TOTAL + 10
#define WARSNET_CLIENT_CHANNEL INetChannelInfo::TOTAL + 11

// Network message types
enum EMessageClient
{
	k_EMsgClientRequestGameAccepted = 0,
	k_EMsgClientRequestGameDenied,
	k_EMsgClient_PyEntityClasses,
	k_EMsgClient_PyEntityUpdate,
};

enum EMessageServer
{
	k_EMsgServerRequestGame = 0,
	k_EMsgLocalServerRequestGame,
	k_EMsgIsLobbyGameActive,
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
	virtual void ReceiveClientSteamP2PMessages( ISteamNetworking *pSteamNetworking ) = 0;
	virtual void ReceiveServerSteamP2PMessages( ISteamNetworking *pSteamNetworking ) = 0;
	virtual WarsMessageData_t *ServerMessageHead() = 0;
	virtual bool NextServerMessage() = 0;
	virtual WarsMessageData_t *InsertServerMessage() = 0;
	virtual WarsMessageData_t *ClientMessageHead() = 0;
	virtual bool NextClientMessage() = 0;
	virtual WarsMessageData_t *InsertClientMessage() = 0;

	// Hack to tell client we are paused (engine->IsPaused is broken...)
	virtual void SetPaused( bool bPaused ) = 0;
	virtual bool IsPaused() = 0;
};

#define WARS_EXTENSION_VERSION		"VWarsExtension001"

#endif // WARS_EXTENSION_INT_H
