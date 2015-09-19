//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef WARSGAMESERVER_H
#define WARSGAMESERVER_H
#ifdef _WIN32
#pragma once
#endif

#include "wars_matchmaking.h"

class CWarsGameServer
{
public:
	CWarsGameServer();

	void PrintDebugInfo();
	void ProcessMessages();
	void UpdateServer();
	void RunFrame();

	void ClientDisconnect( edict_t *pEdict );

	void SetState( EGameServerState state );
	EGameServerState GetState();

	virtual char *GetMatchmakingTags( char *buf, size_t &bufSize );
	virtual char *GetMatchmakingGameData( char *buf, size_t &bufSize );

	// Shutdowns game once all players have left
	void ScheduleShutdown();
	void CancelSheduledShutdown();

	// Returns a valid lobby steam id if the active game was started from a Steam lobby
	CSteamID GetActiveGameLobbySteamID();

	// Returns game data from a local player starting a server
	KeyValues *GetLocalPlayerGameData();

private:
	// Tells us when we have successfully connected to Steam
	STEAM_GAMESERVER_CALLBACK( CWarsGameServer, OnSteamServersConnected, SteamServersConnected_t, m_CallbackSteamServersConnected );

	// Tells us when there was a failure to connect to Steam
	STEAM_GAMESERVER_CALLBACK( CWarsGameServer, OnSteamServersConnectFailure, SteamServerConnectFailure_t, m_CallbackSteamServersConnectFailure );

	// Tells us when we have been logged out of Steam
	STEAM_GAMESERVER_CALLBACK( CWarsGameServer, OnSteamServersDisconnected, SteamServersDisconnected_t, m_CallbackSteamServersDisconnected );

	// Tells us that Steam has set our security policy (VAC on or off)
	STEAM_GAMESERVER_CALLBACK( CWarsGameServer, OnPolicyResponse, GSPolicyResponse_t, m_CallbackPolicyResponse );

	STEAM_GAMESERVER_CALLBACK( CWarsGameServer, OnP2PSessionRequest, P2PSessionRequest_t, m_CallbackP2PSessionRequest );
	STEAM_GAMESERVER_CALLBACK( CWarsGameServer, OnP2PSessionConnectFail, P2PSessionConnectFail_t, m_CallbackP2PSessionConnectFail );
	

	bool m_bConnectedToSteam;
	EGameServerState m_State;
	int m_nConnectedPlayers;
	bool m_bUpdatingServer;

	bool m_bUpdateMatchmakingTags;

	float m_fGameStateStartTime;
	float m_fLastPlayedConnectedTime;
	float m_fGracePeriodEnd;

	bool m_bShutdownScheduled;

	CSteamID m_LobbyPlayerRequestingGameID;

	// Lobby SteamID from this game server was triggered. Players can use this to poll if the game server is still active.
	CSteamID m_ActiveGameLobbySteamID;

	// Game data from a local player starting a server
	KeyValues *m_pLocalPlayerGameData;
};

inline EGameServerState CWarsGameServer::GetState()
{
	return m_State;
}

inline CSteamID CWarsGameServer::GetActiveGameLobbySteamID()
{
	return m_ActiveGameLobbySteamID;
}

inline KeyValues *CWarsGameServer::GetLocalPlayerGameData()
{
	return m_pLocalPlayerGameData;
}

void WarsInitGameServer();
void WarsShutdownGameServer();
EGameServerState GetWarsGameServerState();
void SetWarsGameServerState( EGameServerState state );
CSteamID GetActiveGameLobbySteamID();

CWarsGameServer *WarsGameServer();

#endif // WARSGAMESERVER_H