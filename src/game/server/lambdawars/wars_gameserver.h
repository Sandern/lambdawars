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

class CWarsGameServer
{
public:
	CWarsGameServer();

	void RunFrame();

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

	// lobby state change handler
	STEAM_CALLBACK( CWarsGameServer, OnLobbyDataUpdate, LobbyDataUpdate_t, m_CallbackLobbyDataUpdate );

	bool m_bConnectedToSteam;
};

void WarsInitGameServer();
void WarsShutdownGameServer();
CWarsGameServer *WarsGameServer();

#endif // WARSGAMESERVER_H