//========= Copyright Sandern Corporation, All rights reserved. ============//
#include "cbase.h"
#include "wars_gameserver.h"
#include "gameinterface.h"

#include "matchmaking/imatchframework.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern CServerGameDLL g_ServerGameDLL;

extern const char *COM_GetModDirectory( void );

static CWarsGameServer *g_pGameServerTest = NULL;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWarsGameServer::CWarsGameServer() :
	m_bConnectedToSteam(false),
	m_CallbackP2PSessionRequest( this, &CWarsGameServer::OnP2PSessionRequest ),
	m_CallbackSteamServersConnected( this, &CWarsGameServer::OnSteamServersConnected ),
	m_CallbackSteamServersDisconnected( this, &CWarsGameServer::OnSteamServersDisconnected ),
	m_CallbackSteamServersConnectFailure( this, &CWarsGameServer::OnSteamServersConnectFailure ),
	m_CallbackPolicyResponse( this, &CWarsGameServer::OnPolicyResponse ),
	m_CallbackLobbyDataUpdate( this, &CWarsGameServer::OnLobbyDataUpdate ),
	m_State( k_EGameServer_Available ),
	m_fGameStateStartTime( 0.0f ),
	m_fLastPlayedConnectedTime( 0.0f )
{
	DevMsg("Created Wars game server\n");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWarsGameServer::PrintDebugInfo()
{
	Msg( "State: %d\n", m_State );
	Msg( "Current time: %f\n", Plat_FloatTime() );
	Msg( "Connected players: %d\n", m_nConnectedPlayers );
	Msg( "Last players activity time: %f\n", m_fLastPlayedConnectedTime );
}

//-----------------------------------------------------------------------------
// Purpose: Updates a wars game server
//-----------------------------------------------------------------------------
void CWarsGameServer::RunFrame()
{
	char *pchRecvBuf = NULL;
	uint32 cubMsgSize;
	CSteamID steamIDRemote;

	KeyValues *pData = NULL;

	if( steamgameserverapicontext->SteamGameServer() )
	{
		//steamgameserverapicontext->SteamGameServer()->EnableHeartbeats( true );
		//steamgameserverapicontext->SteamGameServer()->ForceHeartbeat();
	}
	else
	{
		Warning("No steam game server interface\n");
	}

	/*if( steamgameserverapicontext->SteamGameServer() )
		
	else
		*/

	if( GetState() == k_EGameServer_InGame )
	{
		int nConnected = 0;
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );
		
			if ( pPlayer && pPlayer->IsConnected() )
			{
				nConnected++;
			}
		}
		m_nConnectedPlayers = nConnected;

		if( m_nConnectedPlayers > 0 )
		{
			m_fLastPlayedConnectedTime = Plat_FloatTime();
		}
		else if( Plat_FloatTime() - m_fLastPlayedConnectedTime > 60.0f )
		{
			SetState( k_EGameServer_Available );
			Msg("Changing wars game server back to available due inactivity (no connected players)\n");
		}
	}

	// Update state
	steamgameserverapicontext->SteamGameServer()->SetKeyValue( "available", m_State == k_EGameServer_Available ? "1" : "0" );

	while ( steamgameserverapicontext->SteamGameServerNetworking()->IsP2PPacketAvailable( &cubMsgSize ) )
	{
		// free any previous receive buffer
		if ( pchRecvBuf )
			free( pchRecvBuf );

		// alloc a new receive buffer of the right size
		pchRecvBuf = (char *)malloc( cubMsgSize );

		// see if there is any data waiting on the socket
		if ( !steamgameserverapicontext->SteamGameServerNetworking()->ReadP2PPacket( pchRecvBuf, cubMsgSize, &cubMsgSize, &steamIDRemote ) )
			break;

		if ( cubMsgSize < sizeof( uint32 ) )
		{
			Warning( "Got garbage on server socket, too short\n" );
			continue;
		}

		EMessage eMsg = (EMessage)( *(uint32*)pchRecvBuf );
		Msg("Received message of type %d\n", eMsg);

		static WarsMessage_t acceptGameMsg( k_EMsgClientRequestGameAccepted );
		static WarsMessage_t denyGameMsg( k_EMsgClientRequestGameDenied );

		switch( eMsg )
		{
		case k_EMsgServerRequestGame:
			if( GetState() != k_EGameServer_Available )
			{
				// Tell lobby owner the server is not available and should look for another server
				steamgameserverapicontext->SteamGameServerNetworking()->SendP2PPacket( steamIDRemote, &denyGameMsg, sizeof(denyGameMsg), k_EP2PSendReliable );
			}
			else
			{
				// Tell lobby owner the game is accepted and players can connect to the server
				steamgameserverapicontext->SteamGameServerNetworking()->SendP2PPacket( steamIDRemote, &acceptGameMsg, sizeof(acceptGameMsg), k_EP2PSendReliable );

				// Setup game
				pData = KeyValues::FromString(
				"settings",
				" system { "
				" network LIVE "
				" access public "
				" } "
				" game { "
				" mode annihilation "
				" mission hlw_forest "
				" } "
				);
				pData->SetName( COM_GetModDirectory() );
				g_ServerGameDLL.ApplyGameSettings( pData );

				//g_pMatchFramework->CreateSession( pData );

				SetState( k_EGameServer_InGame );
			}
			break;
		default:
			Warning("Game server received unknown/unsupported message\n");
			break;
		}
	}

	if( pData )
		pData->deleteThis();
	if ( pchRecvBuf )
		free( pchRecvBuf );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWarsGameServer::SetState( EGameServerState state )
{
	if( m_State == state ) 
	{
		return;
	}

	m_State = state;
	m_fGameStateStartTime = Plat_FloatTime();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWarsGameServer::GetMatchmakingTags( char *buf, size_t bufSize )
{
	int len = 0;
	if( m_State == k_EGameServer_Available )
	{
		Q_strncpy( buf, "Available,", bufSize );
		len = strlen( buf );
		buf += len;
		bufSize -= len;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handle clients connecting
//-----------------------------------------------------------------------------
void CWarsGameServer::OnP2PSessionRequest( P2PSessionRequest_t *pCallback )
{
	//Msg("Accepting OnP2PSessionRequest\n");
	// we'll accept a connection from anyone
	steamgameserverapicontext->SteamGameServerNetworking()->AcceptP2PSessionWithUser( pCallback->m_steamIDRemote );
}

//-----------------------------------------------------------------------------
// Purpose: Take any action we need to on Steam notifying us we are now logged in
//-----------------------------------------------------------------------------
void CWarsGameServer::OnSteamServersConnected( SteamServersConnected_t *pLogonSuccess )
{
	//Msg( "WarsGameServer connected to Steam successfully\n" );
	m_bConnectedToSteam = true;

	// log on is not finished until OnPolicyResponse() is called

	// Tell Steam about our server details
	//SendUpdatedServerDetailsToSteam();
}

//-----------------------------------------------------------------------------
// Purpose: Called when we were previously logged into steam but get logged out
//-----------------------------------------------------------------------------
void CWarsGameServer::OnSteamServersDisconnected( SteamServersDisconnected_t *pLoggedOff )
{
	m_bConnectedToSteam = false;
	//Msg( "WarsGameServer got logged out of Steam\n" );
}

//-----------------------------------------------------------------------------
// Purpose: Called when an attempt to login to Steam fails
//-----------------------------------------------------------------------------
void CWarsGameServer::OnSteamServersConnectFailure( SteamServerConnectFailure_t *pConnectFailure )
{
	m_bConnectedToSteam = false;
	//Warning( "SpaceWarServer failed to connect to Steam %d\n", pConnectFailure->m_eResult );
}

//-----------------------------------------------------------------------------
// Purpose: Callback from Steam3 when logon is fully completed and VAC secure policy is set
//-----------------------------------------------------------------------------
void CWarsGameServer::OnPolicyResponse( GSPolicyResponse_t *pPolicyResponse )
{
#if 0
	// Check if we were able to go VAC secure or not
	if ( steamgameserverapicontext->SteamGameServer()->BSecure() )
	{
		Msg( "WarsGameServer is VAC Secure!\n" );
	}
	else
	{
		Msg( "WarsGameServer is not VAC Secure!\n" );
	}
	char rgch[128];
	_snprintf( rgch, 128, "Game server SteamID: %llu\n", steamgameserverapicontext->SteamGameServer()->GetSteamID().ConvertToUint64() );
	Msg( rgch );
#endif // 0
}

//-----------------------------------------------------------------------------
// Purpose: Handles lobby data changing
//-----------------------------------------------------------------------------
void CWarsGameServer::OnLobbyDataUpdate( LobbyDataUpdate_t *pCallback )
{
	Msg("WarsGameServer: got lobby data update\n");
	// callbacks are broadcast to all listeners, so we'll get this for every lobby we're requesting
	//if ( m_steamIDLobby != pCallback->m_ulSteamIDLobby )
	//	return;

}

void WarsInitGameServer()
{
	g_pGameServerTest = new CWarsGameServer();
}

void WarsShutdownGameServer()
{
	delete g_pGameServerTest;
	g_pGameServerTest = NULL;
}

CWarsGameServer *WarsGameServer()
{
	return g_pGameServerTest;
}

CON_COMMAND_F( wars_gameserver_info, "", 0 )
{
	if( !WarsGameServer() )
	{
		Msg("No running wars game server\n");
		return;
	}

	WarsGameServer()->PrintDebugInfo();
}