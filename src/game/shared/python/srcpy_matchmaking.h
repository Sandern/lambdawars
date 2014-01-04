//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SRCPY_MATCHMAKING_H
#define SRCPY_MATCHMAKING_H
#ifdef _WIN32
#pragma once
#endif

#include "matchmaking/imatchevents.h"
#include "steam/isteammatchmaking.h"

class KeyValues;
class ISearchManager;
class IMatchSearchResult;

// IMatchFramework wrappers
void PyMKCreateSession( KeyValues *pSettings );
void PyMKMatchSession( KeyValues *pSettings );
void PyMKCloseSession();
bool PyMKIsSessionActive();

//=============================================================================
// Search Result
//=============================================================================
class PyMatchSearchResult
{
public:
	PyMatchSearchResult();

	virtual bool IsValid();

	virtual boost::python::object GetOnlineId();

	virtual KeyValues *GetGameDetails();

	virtual bool IsJoinable();
	virtual void Join();

	// Not for python
	void SetMatchResultInternal( IMatchSearchResult *pMatchResult, boost::python::object weakref );

private:
	IMatchSearchResult *m_pMatchSearchResult;
	boost::python::object m_pySearchManagerWeakRef;
};

//=============================================================================
// Search Manager
//=============================================================================
class PySearchManager
{
public:
	PySearchManager();
	~PySearchManager();

	virtual void Destroy();
	virtual bool IsValid();

	virtual void EnableResultsUpdate( bool bEnable, KeyValues *pSearchParams = NULL );

	virtual int GetNumResults();

	virtual boost::python::object GetResultByIndex( int iResultIdx );
	virtual boost::python::object GetResultByOnlineId( boost::python::object xuidResultOnline );

	// Not for python
	void SetSearchManagerInternal( ISearchManager *pSearchManager, boost::python::object weakref );

private:
	ISearchManager *m_pSearchManager;
	boost::python::object m_pyWeakRef;
};

inline bool PySearchManager::IsValid()
{
	return m_pSearchManager != NULL;
}

//=============================================================================
// Events Sink
//=============================================================================
class PyMatchEventsSink : public IMatchEventsSink
{
public:
	~PyMatchEventsSink();

	virtual void OnEvent( KeyValues *pEvent ) {}
};

//=============================================================================
// MatchEventsSubscription
//=============================================================================
class PyMatchEventsSubscription
{
public:
	static void Subscribe( PyMatchEventsSink *sink );
	static void Unsubscribe( PyMatchEventsSink *sink );

	static void BroadcastEvent( KeyValues *pEvent );

	static void RegisterEventData( KeyValues *pEventData );
	static KeyValues * GetEventData( char const *szEventDataKey );
};

//=============================================================================
// MatchSession
//=============================================================================
class PyMatchSession
{
public:
	static KeyValues * GetSessionSystemData();
	static KeyValues * GetSessionSettings();
	static void UpdateSessionSettings( KeyValues *pSettings );
	static void Command( KeyValues *pCommand );
};

//=============================================================================
// MatchSystem
//=============================================================================
class PyMatchSystem
{
public:
	static boost::python::object CreateGameSearchManager( KeyValues *pSettings );
};

//=============================================================================
// SteamMatchmaking
//=============================================================================
class PySteamMatchmaking
{
public:
	static void AddRequestLobbyListDistanceFilter( ELobbyDistanceFilter eLobbyDistanceFilter );
};

#endif // SRCPY_MATCHMAKING_H
