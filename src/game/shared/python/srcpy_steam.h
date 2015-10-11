//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef SRCPY_STEAM_H
#define SRCPY_STEAM_H

#ifdef _WIN32
#pragma once
#endif

#include "steam/steam_api.h"
#include "steam/isteammatchmaking.h"

// Generic macros for callbacks in Python
#define PY_STEAM_CALLBACK_WRAPPER( name, dataclass ) \
class name##Callback \
{ \
public: \
	name##Callback() : m_Callback( this, &name##Callback::On##name##Internal ) \
	{ \
	} \
	virtual void On##name( dataclass *data ) \
	{ \
	} \
private: \
	STEAM_CALLBACK( name##Callback, On##name##Internal, dataclass, m_Callback ); \
}; \
void name##Callback::On##name##Internal( dataclass *data ) \
{ \
	On##name( data ); \
}

#define PY_STEAM_CALLRESULT_WRAPPER( name, dataclass ) \
class name##CallResult \
{ \
public: \
	name##CallResult( SteamAPICall_t steamapicall ) \
	{ \
		m_CallResult.Set( steamapicall, this, &name##CallResult::On##name##Internal ); \
	} \
	virtual void On##name( dataclass *data, bool iofailure ) \
	{ \
	} \
private: \
	void On##name##Internal( dataclass *data, bool bIOFailure ) \
	{ \
		On##name( data, bIOFailure ); \
	} \
	CCallResult<name##CallResult, dataclass> m_CallResult; \
};

// Wrapper functions Matchmaking
boost::python::tuple PyGetLobbyDataByIndex( CSteamID steamIDLobby, int iLobbyData );
bool PySendLobbyChatMsg( CSteamID steamIDLobby, const char *pvMsgBody );
boost::python::tuple PyGetLobbyChatEntry( CSteamID steamIDLobby, int iChatID, CSteamID *pSteamIDUser );

// Wrapper functions UserStats
boost::python::object PyGetStatFloat( const char *name );
boost::python::object PyGetStatInt( const char *name );

// Wrapper functions for GameServer
boost::python::object PySteamUser_GetAuthSessionTicket();
#ifndef CLIENT_DLL
boost::python::object PyGameServer_GetAuthSessionTicket();
#endif // CLIENT_DLL

// Wrapper functions SteamUGC
boost::python::tuple PyGetItemInstallInfo( PublishedFileId_t nPublishedFileID );

// Wrapper functions Matchmaking Servers
class PySteamMatchmakingServerListResponse : public ISteamMatchmakingServerListResponse
{
public:
	virtual void PyServerResponded( int hRequest, int iServer ) {}
	virtual void PyServerFailedToRespond( int hRequest, int iServer ) {}
	virtual void PyRefreshComplete( int hRequest, EMatchMakingServerResponse response ) {}

private:
	// Server has responded ok with updated data
	virtual void ServerResponded( HServerListRequest hRequest, int iServer ) { PyServerResponded( (int)hRequest, iServer ); }

	// Server has failed to respond
	virtual void ServerFailedToRespond( HServerListRequest hRequest, int iServer ) { PyServerFailedToRespond( (int)hRequest, iServer ); }

	// A list refresh you had initiated is now 100% completed
	virtual void RefreshComplete( HServerListRequest hRequest, EMatchMakingServerResponse response ) { PyRefreshComplete( (int)hRequest, response ); }
};

class PySteamMatchmakingPingResponse : public ISteamMatchmakingPingResponse
{
public:
	void ServerResponded( gameserveritem_t &server ) {}
	void ServerFailedToRespond()  {}
};

class PySteamMatchmakingPlayersResponse : public ISteamMatchmakingPlayersResponse
{
public:
	void AddPlayerToList( const char *pchName, int nScore, float flTimePlayed ) {}
	void PlayersFailedToRespond() {}
	void PlayersRefreshComplete() {}
};

class PySteamMatchmakingRulesResponse : public ISteamMatchmakingRulesResponse
{
public:
	void RulesResponded( const char *pchRule, const char *pchValue ) {}
	void RulesFailedToRespond() {}
	void RulesRefreshComplete() {}
};

class pygameserveritem_t : public gameserveritem_t 
{
public:
	const char *GetGameDir() { return m_szGameDir; }
	const char *GetMap() { return m_szMap; }
	const char *GetGameDescription() { return m_szGameDescription; }
	const char *GetGameTags() { return m_szGameTags; }
};

class PySteamMatchmakingServers
{
public:
	int RequestInternetServerList( AppId_t iApp, boost::python::list filters, PySteamMatchmakingServerListResponse *pRequestServersResponse );
	int RequestLANServerList( AppId_t iApp, ISteamMatchmakingServerListResponse *pRequestServersResponse );
	int RequestFriendsServerList( AppId_t iApp, boost::python::list filters, ISteamMatchmakingServerListResponse *pRequestServersResponse );
	int RequestFavoritesServerList( AppId_t iApp, boost::python::list filters, ISteamMatchmakingServerListResponse *pRequestServersResponse );
	int RequestHistoryServerList( AppId_t iApp, boost::python::list filters, ISteamMatchmakingServerListResponse *pRequestServersResponse );
	int RequestSpectatorServerList( AppId_t iApp, boost::python::list filters, ISteamMatchmakingServerListResponse *pRequestServersResponse );

	void ReleaseRequest( int hServerListRequest );

	pygameserveritem_t GetServerDetails( int hRequest, int iServer );

	// --
	int PingServer( uint32 unIP, uint16 usPort, PySteamMatchmakingPingResponse *pRequestServersResponse ); 
	int PlayerDetails( uint32 unIP, uint16 usPort, PySteamMatchmakingPlayersResponse *pRequestServersResponse );
	HServerQuery ServerRules( uint32 unIP, uint16 usPort, PySteamMatchmakingRulesResponse *pRequestServersResponse ); 

	void CancelServerQuery( HServerQuery hServerQuery ); 
};

#endif // SRCPY_STEAM_H