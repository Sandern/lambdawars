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

class WarsGameServer
{
public:
	WarsGameServer();

	void RunFrame();

private:
	// Tells us when we have successfully connected to Steam
	STEAM_GAMESERVER_CALLBACK( WarsGameServer, OnSteamServersConnected, SteamServersConnected_t, m_CallbackSteamServersConnected );

	// Tells us when there was a failure to connect to Steam
	STEAM_GAMESERVER_CALLBACK( WarsGameServer, OnSteamServersConnectFailure, SteamServerConnectFailure_t, m_CallbackSteamServersConnectFailure );

	// Tells us when we have been logged out of Steam
	STEAM_GAMESERVER_CALLBACK( WarsGameServer, OnSteamServersDisconnected, SteamServersDisconnected_t, m_CallbackSteamServersDisconnected );

	// Tells us that Steam has set our security policy (VAC on or off)
	STEAM_GAMESERVER_CALLBACK( WarsGameServer, OnPolicyResponse, GSPolicyResponse_t, m_CallbackPolicyResponse );

	STEAM_GAMESERVER_CALLBACK( WarsGameServer, OnP2PSessionRequest, P2PSessionRequest_t, m_CallbackP2PSessionRequest );

	bool m_bConnectedToSteam;
};

#endif // WARSGAMESERVER_H