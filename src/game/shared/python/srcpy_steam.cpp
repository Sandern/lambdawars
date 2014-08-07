//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
//=============================================================================//

#include "cbase.h"
#include "srcpy_steam.h"
#include "srcpy_boostpython.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

boost::python::tuple PyGetLobbyDataByIndex( CSteamID steamIDLobby, int iLobbyData )
{
	char key[1024];
	char value[1024];

	if( !steamapicontext->SteamMatchmaking() )
	{
		PyErr_SetString(PyExc_Exception, "No steam matchmaking API available!" );
		throw boost::python::error_already_set(); 
	}

	bool ret = steamapicontext->SteamMatchmaking()->GetLobbyDataByIndex( steamIDLobby, iLobbyData, key, sizeof(key), value, sizeof(value) );

	return boost::python::make_tuple( ret, key, value );
}

bool PySendLobbyChatMsg( CSteamID steamIDLobby, const char *pvMsgBody )
{
	if( !steamapicontext->SteamMatchmaking() )
	{
		PyErr_SetString(PyExc_Exception, "No steam matchmaking API available!" );
		throw boost::python::error_already_set(); 
	}

	return steamapicontext->SteamMatchmaking()->SendLobbyChatMsg( steamIDLobby, (const void *)pvMsgBody, V_strlen(pvMsgBody) + 1 );
}

boost::python::tuple PyGetLobbyChatEntry( CSteamID steamIDLobby, int iChatID, CSteamID *pSteamIDUser )
{
	if( !steamapicontext->SteamMatchmaking() )
	{
		PyErr_SetString(PyExc_Exception, "No steam matchmaking API available!" );
		throw boost::python::error_already_set(); 
	}
	
	char data[4096];
	data[0] = '\0';
	EChatEntryType eChatEntryType;
	steamapicontext->SteamMatchmaking()->GetLobbyChatEntry( steamIDLobby, iChatID, pSteamIDUser, data, sizeof( data ), &eChatEntryType );
	return boost::python::make_tuple( data, eChatEntryType );
}

#define STEAM_MM_SERVERS_VALID()	if( !steamapicontext->SteamMatchmakingServers() ) \
	{ \
		PyErr_SetString(PyExc_Exception, "No steam matchmaking servers API available!" ); \
		throw boost::python::error_already_set(); \
	}

int PySteamMatchmakingServers::RequestInternetServerList( AppId_t iApp, boost::python::list filters, PySteamMatchmakingServerListResponse *pRequestServersResponse )
{
	STEAM_MM_SERVERS_VALID();

	int nFilters = boost::python::len( filters );
	MatchMakingKeyValuePair_t **ppchFilters = (MatchMakingKeyValuePair_t **)stackalloc( sizeof( MatchMakingKeyValuePair_t * ) * nFilters );
	for( int i = 0; i < nFilters; i++ )
	{
		ppchFilters[i] = (MatchMakingKeyValuePair_t *)stackalloc( sizeof( MatchMakingKeyValuePair_t ) );
		V_strcpy( ppchFilters[i]->m_szKey, boost::python::extract<const char *>( filters[i][0] ) );
		V_strcpy( ppchFilters[i]->m_szValue, boost::python::extract<const char *>( filters[i][1] ) );
	}

	return (int)steamapicontext->SteamMatchmakingServers()->RequestInternetServerList( iApp, ppchFilters, nFilters, pRequestServersResponse );
}

void PySteamMatchmakingServers::ReleaseRequest( int hServerListRequest )
{
	STEAM_MM_SERVERS_VALID();
	steamapicontext->SteamMatchmakingServers()->ReleaseRequest( reinterpret_cast<HServerListRequest>(hServerListRequest) );
}

gameserveritem_t *PySteamMatchmakingServers::GetServerDetails( int hRequest, int iServer )
{
	STEAM_MM_SERVERS_VALID();
	return steamapicontext->SteamMatchmakingServers()->GetServerDetails( reinterpret_cast<HServerListRequest>(hRequest), iServer );
}

int PySteamMatchmakingServers::PingServer( uint32 unIP, uint16 usPort, PySteamMatchmakingPingResponse *pRequestServersResponse )
{
	STEAM_MM_SERVERS_VALID();
	return steamapicontext->SteamMatchmakingServers()->PingServer( unIP, usPort, pRequestServersResponse );
}

int PySteamMatchmakingServers::PlayerDetails( uint32 unIP, uint16 usPort, PySteamMatchmakingPlayersResponse *pRequestServersResponse )
{
	STEAM_MM_SERVERS_VALID();
	return steamapicontext->SteamMatchmakingServers()->PlayerDetails( unIP, usPort, pRequestServersResponse );
}

HServerQuery PySteamMatchmakingServers::ServerRules( uint32 unIP, uint16 usPort, PySteamMatchmakingRulesResponse *pRequestServersResponse )
{
	STEAM_MM_SERVERS_VALID();
	return steamapicontext->SteamMatchmakingServers()->ServerRules( unIP, usPort, pRequestServersResponse );
}

void PySteamMatchmakingServers::CancelServerQuery( HServerQuery hServerQuery )
{
	STEAM_MM_SERVERS_VALID();
	steamapicontext->SteamMatchmakingServers()->CancelServerQuery( hServerQuery );
}