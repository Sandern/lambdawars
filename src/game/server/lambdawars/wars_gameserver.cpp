//========= Copyright Sandern Corporation, All rights reserved. ============//
#include "cbase.h"
#include "wars_gameserver.h"
#include "gameinterface.h"
#include "tier1/utlbuffer.h"
#include "srcpy_networkvar.h"
#include "wars_network.h"

#include "matchmaking/imatchframework.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar wars_gamserver_debug("wars_gamserver_debug", "0");

extern CServerGameDLL g_ServerGameDLL;

extern const char *COM_GetModDirectory( void );

static CWarsGameServer *g_pGameServerTest = NULL;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWarsGameServer::CWarsGameServer() :
	m_bConnectedToSteam(false),
	m_bUpdatingServer(false),
	m_CallbackP2PSessionRequest( this, &CWarsGameServer::OnP2PSessionRequest ),
	m_CallbackP2PSessionConnectFail( this, &CWarsGameServer::OnP2PSessionConnectFail ),
	m_CallbackSteamServersConnected( this, &CWarsGameServer::OnSteamServersConnected ),
	m_CallbackSteamServersDisconnected( this, &CWarsGameServer::OnSteamServersDisconnected ),
	m_CallbackSteamServersConnectFailure( this, &CWarsGameServer::OnSteamServersConnectFailure ),
	m_CallbackPolicyResponse( this, &CWarsGameServer::OnPolicyResponse ),
	m_State( k_EGameServer_Available ),
	m_fGameStateStartTime( 0.0f ),
	m_fLastPlayedConnectedTime( 0.0f ),
	m_bUpdateMatchmakingTags(true),
	m_pLocalPlayerGameData(NULL),
	m_bShutdownScheduled(false)
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
// Purpose: 
//-----------------------------------------------------------------------------
void CWarsGameServer::ProcessMessages()
{
	ConVarRef hostport("hostport");

	bool bGameServerIsRunning = steamgameserverapicontext->SteamGameServer() != NULL;

	KeyValues *pData = NULL, *pGameData = NULL;

	warsextension->ReceiveSteamP2PMessages( steamgameserverapicontext->SteamGameServerNetworking() );

	// Process server messages
	WarsMessageData_t *messageData = warsextension->ServerMessageHead();
	while( messageData )
	{
		EMessage eMsg = (EMessage)( *(uint32*)messageData->buf.Base() );

		static WarsAcceptGameMessage_t acceptGameMsg( k_EMsgClientRequestGameAccepted );
		static WarsMessage_t denyGameMsg( k_EMsgClientRequestGameDenied );

		CUtlBuffer data;

		if( !bGameServerIsRunning )
		{
			// Special case when local client starts game server from main menu/lobby
			if( eMsg == k_EMsgLocalServerRequestGame )
			{
#if 0 // Currently does not work properly because the server is not cleaned up nicely for local players
				if( GetState() != k_EGameServer_Available )
				{
					// Tell lobby owner the server is not available and should look for another server
					WarsMessageData_t *pMessageData = warsextension->InsertClientMessage();
					pMessageData->buf.Put( &denyGameMsg, sizeof(denyGameMsg) );
					pMessageData->steamIDRemote = messageData->steamIDRemote;
				}
				else
#endif // 0
				{
					WarsRequestServerMessage_t *requestMsg = (WarsRequestServerMessage_t *)messageData->buf.Base();

					data.Put( (char *)messageData->buf.Base() + sizeof( WarsRequestServerMessage_t ), messageData->buf.TellMaxPut() - sizeof( WarsRequestServerMessage_t ) );
					pGameData = new KeyValues( "GameData" );
					if( !pGameData->ReadAsBinary( data ) )
					{
						Warning("k_EMsgServerRequestGame: reading game data failed\n");
						WarsMessageData_t *pMessageData = warsextension->InsertClientMessage();
						pMessageData->buf.Put( &denyGameMsg, sizeof(denyGameMsg) );
						pMessageData->steamIDRemote = messageData->steamIDRemote;

					}
					else
					{
						pGameData->SetName( COM_GetModDirectory() );

						KeyValuesDumpAsDevMsg( pGameData, 0, 0 );

						m_pLocalPlayerGameData = pGameData->MakeCopy();

						// Tell lobby owner the game is accepted and players can connect to the server
						//WarsMessageData_t *pMessageData = warsextension->InsertClientMessage();
						//pMessageData->buf.Put( &acceptGameMsg, sizeof(acceptGameMsg) );
						//pMessageData->steamIDRemote = messageData->steamIDRemote;

						//g_ServerGameDLL.ApplyGameSettings( pGameData );

						SetState( k_EGameServer_StartingGame );
						m_LobbyPlayerRequestingGameID = messageData->steamIDRemote;

						m_ActiveGameLobbySteamID =  requestMsg->lobbySteamId;
					}
				}
			}
			else
			{
				Warning("Receiving game server message %d while not running. Discarding...\n", eMsg);
			}
		}
		else
		{
			switch( eMsg )
			{
			case k_EMsgServerRequestGame:
				if( GetState() != k_EGameServer_Available )
				{
					// Tell lobby owner the server is not available and should look for another server
					steamgameserverapicontext->SteamGameServerNetworking()->SendP2PPacket( messageData->steamIDRemote, &denyGameMsg, sizeof(denyGameMsg), k_EP2PSendReliable );
				}
				else
				{
					WarsRequestServerMessage_t *requestMsg = (WarsRequestServerMessage_t *)messageData->buf.Base();

					data.Put( (char *)messageData->buf.Base() + sizeof( WarsRequestServerMessage_t ), messageData->buf.TellMaxPut() - sizeof( WarsRequestServerMessage_t ) );
					pGameData = new KeyValues( "GameData" );
					if( !pGameData->ReadAsBinary( data ) )
					{
						Warning("k_EMsgServerRequestGame: reading game data failed\n");
						steamgameserverapicontext->SteamGameServerNetworking()->SendP2PPacket( messageData->steamIDRemote, &denyGameMsg, sizeof(denyGameMsg), k_EP2PSendReliable );

					}
					else
					{
						SetState( k_EGameServer_InGame );
						m_ActiveGameLobbySteamID = requestMsg->lobbySteamId;

						pGameData->SetName( COM_GetModDirectory() );
						
						Msg("Starting wars game server with following settings: ");
						KeyValuesDumpAsDevMsg( pGameData, 0, 0 );

						acceptGameMsg.publicIP = steamgameserverapicontext->SteamGameServer()->GetPublicIP();
						acceptGameMsg.gamePort = hostport.GetInt();
						acceptGameMsg.serverSteamID = steamgameserverapicontext->SteamGameServer()->GetSteamID().ConvertToUint64();

						// Tell lobby owner the game is accepted and players can connect to the server
						steamgameserverapicontext->SteamGameServerNetworking()->SendP2PPacket( messageData->steamIDRemote, &acceptGameMsg, sizeof(acceptGameMsg), k_EP2PSendReliable );
						g_ServerGameDLL.ApplyGameSettings( pGameData );
					}
				}
				break;
			default:
				Warning("Game server received unknown/unsupported message type %d\n", eMsg);
				break;
			}
		}

		warsextension->NextServerMessage();
		messageData = warsextension->ServerMessageHead();
	}

	if( pData )
		pData->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWarsGameServer::UpdateServer()
{
	//steamgameserverapicontext->SteamGameServer()->EnableHeartbeats( true );
	//steamgameserverapicontext->SteamGameServer()->SetHeartbeatInterval( -1 );
	//steamgameserverapicontext->SteamGameServer()->ForceHeartbeat();

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

	switch( GetState() )
	{
		case k_EGameServer_StartingGame:
		{
			if( steamgameserverapicontext->SteamGameServer()->GetPublicIP() != 0 )
			{
				if( m_LobbyPlayerRequestingGameID.IsValid() ) 
				{
					ConVarRef hostport("hostport");

					WarsAcceptGameMessage_t acceptGameMsg( k_EMsgClientRequestGameAccepted );
					acceptGameMsg.publicIP = steamgameserverapicontext->SteamGameServer()->GetPublicIP();
					acceptGameMsg.gamePort = hostport.GetInt();
					acceptGameMsg.serverSteamID = steamgameserverapicontext->SteamGameServer()->GetSteamID().ConvertToUint64();

					WarsMessageData_t *pMessageData = warsextension->InsertClientMessage();
					pMessageData->buf.Put( &acceptGameMsg, sizeof(acceptGameMsg) );
					pMessageData->steamIDRemote = m_LobbyPlayerRequestingGameID;
				}

				m_LobbyPlayerRequestingGameID.Clear();
				if( m_pLocalPlayerGameData )
				{
					m_pLocalPlayerGameData->deleteThis();
					m_pLocalPlayerGameData = NULL;
				}
				SetState( k_EGameServer_InGame );
			}
			break;
		}
		case k_EGameServer_InGame:
		case k_EGameServer_InGameFreeStyle:
		{
			if( m_nConnectedPlayers > 0 )
			{
				m_fLastPlayedConnectedTime = Plat_FloatTime();
			}
			else if( Plat_FloatTime() - m_fLastPlayedConnectedTime > 60.0f )
			{
				SetState( k_EGameServer_Available );
				Msg("Changing wars game server back to available due inactivity (no connected players)\n");
			}
			break;
		}
		case k_EGameServer_GameEnded:
		{
			// Game ended just servers as a way to directly become available again after no players are connected any more
			// k_EGameServer_InGame uses a time out before doing so.
			if( m_nConnectedPlayers == 0 )
			{
				SetState( k_EGameServer_Available );
				Msg("Changing wars game server back to available because game ended and no players are connected anymore\n");
			}
			break;
		}
		case k_EGameServer_Available:
		{
			if( nConnected > 0 )
			{
				SetState( k_EGameServer_InGameFreeStyle );
				Msg("Changing wars game server to free style ingame since players are connected\n");
			}
			else
			{
				m_fLastPlayedConnectedTime = Plat_FloatTime();
			}
			break;
		}
		default:
		{
			break;
		}
	}

	// Update state
	steamgameserverapicontext->SteamGameServer()->SetKeyValue( "available", m_State == k_EGameServer_Available ? "1" : "0" );

	ProcessMessages();

	// Update tags for matchmaking if needed
	if( m_bUpdateMatchmakingTags ) 
	{
		m_bUpdateMatchmakingTags = false;

		char buf[1024];
		buf[0] = 0;
		g_ServerGameDLL.GetMatchmakingTags( buf, sizeof(buf) );

		steamgameserverapicontext->SteamGameServer()->SetGameTags( buf );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates a wars game server
//-----------------------------------------------------------------------------
void CWarsGameServer::RunFrame()
{
	if( !steamgameserverapicontext->SteamGameServer() )
	{
		ProcessMessages();
		//Warning("No steam game server interface\n");
		return;
	}
	if( !warsextension )
	{
		Error("No steam warsextension interface\n");
		return;
	}

	if( !m_bUpdatingServer )
	{
		// ApplyGameSettings will do a RunFrame, but this gives undesired behaviour when
		// the server is started from inside RunFrame
		m_bUpdatingServer = true;
		UpdateServer();
		m_bUpdatingServer = false;
	}
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

	if( wars_gamserver_debug.GetBool() ) 
	{
		Msg( "WarsGameserver state changed from %d to %d\n", m_State, state );
	}

	m_State = state;
	m_fGameStateStartTime = Plat_FloatTime();
	m_bUpdateMatchmakingTags = true;

	if( state == k_EGameServer_Available )
	{
		m_ActiveGameLobbySteamID.Clear();
		if( m_pLocalPlayerGameData )
		{
			m_pLocalPlayerGameData->deleteThis();
			m_pLocalPlayerGameData = NULL;
		}

		if( m_bShutdownScheduled )
		{
			engine->ServerCommand( "exit\n" );
			m_bShutdownScheduled = false;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
char *CWarsGameServer::GetMatchmakingTags( char *buf, size_t bufSize )
{
	int len = 0;
	if( m_State == k_EGameServer_Available )
	{
		V_strncpy( buf, "Available,", bufSize );
		len = V_strlen( buf );
		buf += len;
		bufSize -= len;
	}

	return buf;
}

//-----------------------------------------------------------------------------
// Purpose: Handle clients connecting
//-----------------------------------------------------------------------------
void CWarsGameServer::OnP2PSessionRequest( P2PSessionRequest_t *pCallback )
{
	// we'll accept a connection from anyone
	steamgameserverapicontext->SteamGameServerNetworking()->AcceptP2PSessionWithUser( pCallback->m_steamIDRemote );
}

//-----------------------------------------------------------------------------
// Purpose: Handle errors
//-----------------------------------------------------------------------------
void CWarsGameServer::OnP2PSessionConnectFail( P2PSessionConnectFail_t *pCallback )
{
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if( !pPlayer )
			continue;

		const CSteamID *playerSteamId = engine->GetClientSteamID( pPlayer->edict() );
		if( !playerSteamId || !playerSteamId->IsValid() || *playerSteamId != pCallback->m_steamIDRemote )
			continue;
		
		// Player specific warning
		Warning( "Resetting Python networked vars for player %s due p2p session connect fail (%s)\n", 
			pPlayer->GetPlayerName(), WarsNet_TranslateP2PConnectErr( pCallback->m_eP2PSessionError ) );
#if defined(USE_WARS_NETWORK)
		PyNetworkVarsResetClientTransmitBits( i - 1 );
#endif // USE_WARS_NETWORK
		return;
	}

	// General warning
	Warning( "CWarsGameServer::OnP2PSessionConnectFail: %d\n", WarsNet_TranslateP2PConnectErr( pCallback->m_eP2PSessionError ) );
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
// Purpose: 
//-----------------------------------------------------------------------------
void CWarsGameServer::ScheduleShutdown()
{
	if( !engine->IsDedicatedServer() )
	{
		Msg("Only dedicated servers can schedule shutdowns\n");
		return;
	}

	if( GetState() == k_EGameServer_Available )
	{
		engine->ServerCommand( "exit\n" );
		return;
	}

	m_bShutdownScheduled = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWarsGameServer::CancelSheduledShutdown()
{
	m_bShutdownScheduled = false;
}

void WarsInitGameServer()
{
	if( g_pGameServerTest )
		return;

	g_pGameServerTest = new CWarsGameServer();
}

void WarsShutdownGameServer()
{
	if( !g_pGameServerTest )
		return;

	delete g_pGameServerTest;
	g_pGameServerTest = NULL;
}

EGameServerState GetWarsGameServerState()
{
	if( !WarsGameServer() )
		return k_EGameServer_Error;
	return WarsGameServer()->GetState();
}

void SetWarsGameServerState( EGameServerState state )
{
	if( !WarsGameServer() )
		return;
	return WarsGameServer()->SetState( state );
}

CSteamID GetActiveGameLobbySteamID()
{
	if( !WarsGameServer() )
		return CSteamID();
	return WarsGameServer()->GetActiveGameLobbySteamID();
}

CWarsGameServer *WarsGameServer()
{
	return g_pGameServerTest;
}

//-----------------------------------------------------------------------------
// Purpose: Wars Game Server commands
//-----------------------------------------------------------------------------
CON_COMMAND_F( wars_gameserver_info, "", 0 )
{
	if( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if( !WarsGameServer() )
	{
		Msg("No running wars game server\n");
		return;
	}

	WarsGameServer()->PrintDebugInfo();
}

CON_COMMAND_F( wars_gameserver_force_available, "", 0 )
{
	if( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if( !WarsGameServer() )
	{
		Msg("No running wars game server\n");
		return;
	}

	WarsGameServer()->SetState( k_EGameServer_Available );
}

CON_COMMAND_F( wars_gameserver_start, "", 0 )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	WarsInitGameServer();
}

CON_COMMAND_F( wars_gameserver_scheduleshutdown, "", 0 )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if( WarsGameServer() )
	{
		WarsGameServer()->ScheduleShutdown();
	}
}

CON_COMMAND_F( wars_gameserver_cancelscheduledshutdown, "", 0 )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if( WarsGameServer() )
	{
		WarsGameServer()->CancelSheduledShutdown();
	}
}