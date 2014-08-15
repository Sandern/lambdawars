//========= Copyright Sandern Corporation, All rights reserved. ============//
#include "cbase.h"
#include "wars_gameserver.h"
#include "wars_matchmaking.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
WarsGameServer::WarsGameServer() :
	m_bConnectedToSteam(false),
	m_CallbackP2PSessionRequest( this, &WarsGameServer::OnP2PSessionRequest ),
	m_CallbackSteamServersConnected( this, &WarsGameServer::OnSteamServersConnected ),
	m_CallbackSteamServersDisconnected( this, &WarsGameServer::OnSteamServersDisconnected ),
	m_CallbackSteamServersConnectFailure( this, &WarsGameServer::OnSteamServersConnectFailure ),
	m_CallbackPolicyResponse( this, &WarsGameServer::OnPolicyResponse )
{
	
}

//-----------------------------------------------------------------------------
// Purpose: Updates a wars game server
//-----------------------------------------------------------------------------
void WarsGameServer::RunFrame()
{
	char *pchRecvBuf = NULL;
	uint32 cubMsgSize;
	CSteamID steamIDRemote;

	if( steamgameserverapicontext->SteamGameServer() )
		steamgameserverapicontext->SteamGameServer()->EnableHeartbeats( true );
	else
		Warning("No steam game server interface\n");

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
	}

	if ( pchRecvBuf )
		free( pchRecvBuf );
}

//-----------------------------------------------------------------------------
// Purpose: Handle clients connecting
//-----------------------------------------------------------------------------
void WarsGameServer::OnP2PSessionRequest( P2PSessionRequest_t *pCallback )
{
	//Msg("Accepting OnP2PSessionRequest\n");
	// we'll accept a connection from anyone
	steamgameserverapicontext->SteamGameServerNetworking()->AcceptP2PSessionWithUser( pCallback->m_steamIDRemote );
}

//-----------------------------------------------------------------------------
// Purpose: Take any action we need to on Steam notifying us we are now logged in
//-----------------------------------------------------------------------------
void WarsGameServer::OnSteamServersConnected( SteamServersConnected_t *pLogonSuccess )
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
void WarsGameServer::OnSteamServersDisconnected( SteamServersDisconnected_t *pLoggedOff )
{
	m_bConnectedToSteam = false;
	//Msg( "WarsGameServer got logged out of Steam\n" );
}

//-----------------------------------------------------------------------------
// Purpose: Called when an attempt to login to Steam fails
//-----------------------------------------------------------------------------
void WarsGameServer::OnSteamServersConnectFailure( SteamServerConnectFailure_t *pConnectFailure )
{
	m_bConnectedToSteam = false;
	//Warning( "SpaceWarServer failed to connect to Steam %d\n", pConnectFailure->m_eResult );
}

//-----------------------------------------------------------------------------
// Purpose: Callback from Steam3 when logon is fully completed and VAC secure policy is set
//-----------------------------------------------------------------------------
void WarsGameServer::OnPolicyResponse( GSPolicyResponse_t *pPolicyResponse )
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