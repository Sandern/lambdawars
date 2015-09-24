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

boost::python::object PyGetStatFloat( const char *name )
{
	if( !steamapicontext->SteamUserStats() )
	{
		PyErr_SetString(PyExc_Exception, "No steam user stats API available!" );
		throw boost::python::error_already_set(); 
	}

	float stat;
	if( !steamapicontext->SteamUserStats()->GetStat( name, &stat ) )
	{
		return boost::python::object();
	}
	return boost::python::object( stat );
}

boost::python::object PyGetStatInt( const char *name )
{
	if( !steamapicontext->SteamUserStats() )
	{
		PyErr_SetString(PyExc_Exception, "No steam user stats API available!" );
		throw boost::python::error_already_set(); 
	}

	int32 stat;
	if( !steamapicontext->SteamUserStats()->GetStat( name, &stat ) )
	{
		return boost::python::object();
	}
	return boost::python::object( stat );
}

boost::python::tuple PyGetItemInstallInfo( PublishedFileId_t nPublishedFileID )
{
	if( !steamapicontext->SteamUGC() )
	{
		PyErr_SetString(PyExc_Exception, "No steam ugc API available!" );
		throw boost::python::error_already_set(); 
	}

	uint64 punSizeOnDisk;
	uint32 punTimeStamp;
	char path[MAX_PATH];
	bool ret = steamapicontext->SteamUGC()->GetItemInstallInfo( nPublishedFileID, &punSizeOnDisk, path, sizeof( path ), &punTimeStamp );
	return boost::python::make_tuple( ret, punSizeOnDisk, path, punTimeStamp );
}

#define STEAM_MM_SERVERS_VALID()	if( !steamapicontext->SteamMatchmakingServers() ) \
	{ \
		PyErr_SetString(PyExc_Exception, "No steam matchmaking servers API available!" ); \
		throw boost::python::error_already_set(); \
	}

#define STEAM_MM_PROCESS_FILTERS()	uint32 nFilters = boost::python::len( filters );	\
	MatchMakingKeyValuePair_t *pFilters = (MatchMakingKeyValuePair_t *)stackalloc( sizeof( MatchMakingKeyValuePair_t ) * nFilters ); \
	for( uint32 i = 0; i < nFilters; i++ )	\
	{	\
		V_strncpy( pFilters[i].m_szKey, boost::python::extract<const char *>( filters[i][0] ), sizeof(pFilters[i].m_szKey) );	\
		V_strncpy( pFilters[i].m_szValue, boost::python::extract<const char *>( filters[i][1] ), sizeof(pFilters[i].m_szValue) );	\
	}

int PySteamMatchmakingServers::RequestInternetServerList( AppId_t iApp, boost::python::list filters, PySteamMatchmakingServerListResponse *pRequestServersResponse )
{
	STEAM_MM_SERVERS_VALID();
	STEAM_MM_PROCESS_FILTERS();

	return (int)steamapicontext->SteamMatchmakingServers()->RequestInternetServerList( iApp, &pFilters, nFilters, pRequestServersResponse );
}

int PySteamMatchmakingServers::RequestLANServerList( AppId_t iApp, ISteamMatchmakingServerListResponse *pRequestServersResponse )
{
	STEAM_MM_SERVERS_VALID();

	return (int)steamapicontext->SteamMatchmakingServers()->RequestLANServerList( iApp, pRequestServersResponse );
}

int PySteamMatchmakingServers::RequestFriendsServerList( AppId_t iApp, boost::python::list filters, ISteamMatchmakingServerListResponse *pRequestServersResponse )
{
	STEAM_MM_SERVERS_VALID();
	STEAM_MM_PROCESS_FILTERS();

	return (int)steamapicontext->SteamMatchmakingServers()->RequestFriendsServerList( iApp, &pFilters, nFilters, pRequestServersResponse );
}

int PySteamMatchmakingServers::RequestFavoritesServerList( AppId_t iApp, boost::python::list filters, ISteamMatchmakingServerListResponse *pRequestServersResponse )
{
	STEAM_MM_SERVERS_VALID();
	STEAM_MM_PROCESS_FILTERS();

	return (int)steamapicontext->SteamMatchmakingServers()->RequestFavoritesServerList( iApp, &pFilters, nFilters, pRequestServersResponse );
}

int PySteamMatchmakingServers::RequestHistoryServerList( AppId_t iApp, boost::python::list filters, ISteamMatchmakingServerListResponse *pRequestServersResponse )
{
	STEAM_MM_SERVERS_VALID();
	STEAM_MM_PROCESS_FILTERS();

	return (int)steamapicontext->SteamMatchmakingServers()->RequestHistoryServerList( iApp, &pFilters, nFilters, pRequestServersResponse );
}

int PySteamMatchmakingServers::RequestSpectatorServerList( AppId_t iApp, boost::python::list filters, ISteamMatchmakingServerListResponse *pRequestServersResponse )
{
	STEAM_MM_SERVERS_VALID();
	STEAM_MM_PROCESS_FILTERS();

	return (int)steamapicontext->SteamMatchmakingServers()->RequestSpectatorServerList( iApp, &pFilters, nFilters, pRequestServersResponse );
}

void PySteamMatchmakingServers::ReleaseRequest( int hServerListRequest )
{
	STEAM_MM_SERVERS_VALID();
	steamapicontext->SteamMatchmakingServers()->ReleaseRequest( reinterpret_cast<HServerListRequest>(hServerListRequest) );
}

pygameserveritem_t PySteamMatchmakingServers::GetServerDetails( int hRequest, int iServer )
{
	STEAM_MM_SERVERS_VALID();
	gameserveritem_t *details = steamapicontext->SteamMatchmakingServers()->GetServerDetails( reinterpret_cast<HServerListRequest>(hRequest), iServer );

	pygameserveritem_t pydetails;
	V_memcpy( &pydetails, details, sizeof(gameserveritem_t) );
	return pydetails;
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