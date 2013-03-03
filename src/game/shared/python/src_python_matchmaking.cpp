//====== Copyright © 2013 Sandern Corporation, All rights reserved. ===========//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "src_python.h"
#include "src_python_matchmaking.h"
#include "matchmaking/imatchframework.h"
#include "matchmaking/isearchmanager.h"
#include "steam/steam_api.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define MATCHSESSION_VALID	\
{ \
	IMatchSession* pMatchSession = g_pMatchFramework->GetMatchSession(); \
	if( !pMatchSession ) \
	{ \
		PyErr_SetString(PyExc_ValueError, "No match session active!"); \
		throw boost::python::error_already_set(); \
	} \
}

//=============================================================================
// Match Result
//=============================================================================
PyMatchSearchResult::PyMatchSearchResult() 
	: m_pMatchSearchResult( NULL )
{
}

bool PyMatchSearchResult::IsValid()
{
	return m_pySearchManagerWeakRef().ptr() != Py_None;
}

boost::python::object PyMatchSearchResult::GetOnlineId()
{
	if( !IsValid() )
		return bp::object();

	XUID onlineid = m_pMatchSearchResult->GetOnlineId();

	PyObject *pOnlineID = PyLong_FromUnsignedLongLong( onlineid );
	if( !pOnlineID )
		return bp::object();

	boost::python::object pyonlineid = boost::python::object(
		boost::python::handle<>(
			boost::python::borrowed( pOnlineID )
		)
	);

	return pyonlineid;
}

KeyValues *PyMatchSearchResult::GetGameDetails()
{
	if( !IsValid() )
		return NULL;

	return m_pMatchSearchResult->GetGameDetails();
}

bool PyMatchSearchResult::IsJoinable()
{
	if( !IsValid() )
		return false;

	return m_pMatchSearchResult->IsJoinable();
}

void PyMatchSearchResult::Join()
{
	if( !IsValid() )
		return;

	m_pMatchSearchResult->Join();
}

void PyMatchSearchResult::SetMatchResultInternal( IMatchSearchResult *pMatchResult, boost::python::object weakref )
{
	m_pMatchSearchResult = pMatchResult;
	m_pySearchManagerWeakRef = weakref;
}

//=============================================================================
// Search Manager
//=============================================================================
PySearchManager::PySearchManager() : m_pSearchManager(NULL)
{
}

PySearchManager::~PySearchManager()
{
	this->Destroy();
}

void PySearchManager::Destroy()
{
	if( m_pSearchManager )
	{
		m_pSearchManager->Destroy();
		m_pSearchManager = NULL;
	}
}

void PySearchManager::EnableResultsUpdate( bool bEnable, KeyValues *pSearchParams )
{
	if( !m_pSearchManager || !pSearchParams )
		return;

	m_pSearchManager->EnableResultsUpdate( bEnable, pSearchParams );
}

int PySearchManager::GetNumResults()
{
	if( !m_pSearchManager )
		return -1;
	return m_pSearchManager->GetNumResults();
}

boost::python::object PySearchManager::GetResultByIndex( int iResultIdx )
{
	if( !m_pSearchManager )
		return bp::object();

	IMatchSearchResult *pResult = m_pSearchManager->GetResultByIndex( iResultIdx );
	if( !pResult )
		return bp::object();

	bp::object matchresult = matchmaking.attr("MatchSearchResult")();

	PyMatchSearchResult *pPyMatchResult = bp::extract<PyMatchSearchResult *>( matchresult );
	if( !pPyMatchResult )
		return bp::object();

	pPyMatchResult->SetMatchResultInternal( pResult, m_pyWeakRef );

	return matchresult;
}

boost::python::object PySearchManager::GetResultByOnlineId( boost::python::object pyXUIDResultOnline )
{
	if( !m_pSearchManager )
		return bp::object();

	XUID xuidResultOnline = PyLong_AsUnsignedLongLongMask( pyXUIDResultOnline.ptr() );

	IMatchSearchResult *pResult = m_pSearchManager->GetResultByOnlineId( xuidResultOnline );
	if( !pResult )
		return bp::object();

	bp::object matchresult = matchmaking.attr("MatchSearchResult")();

	PyMatchSearchResult *pPyMatchResult = bp::extract<PyMatchSearchResult *>( matchresult );
	if( !pPyMatchResult )
		return bp::object();

	pPyMatchResult->SetMatchResultInternal( pResult, m_pyWeakRef );

	return matchresult;
}

void PySearchManager::SetSearchManagerInternal( ISearchManager *pSearchManager, boost::python::object weakref )
{
	m_pSearchManager = pSearchManager;
	m_pyWeakRef = weakref;
}

//=============================================================================
// MatchEventsSink
//=============================================================================
PyMatchEventsSink::~PyMatchEventsSink()
{
	PyMatchEventsSubscription::Unsubscribe( this );
}

//=============================================================================
// MatchFramework
//=============================================================================
void PyMKCreateSession( KeyValues *pSettings )
{
	g_pMatchFramework->CreateSession( pSettings );
}

void PyMKMatchSession( KeyValues *pSettings )
{
	g_pMatchFramework->MatchSession( pSettings );
}

void PyMKCloseSession()
{
	g_pMatchFramework->CloseSession();
}

//=============================================================================
// MatchEventsSubscription
//=============================================================================
void PyMatchEventsSubscription::Subscribe( PyMatchEventsSink *sink )
{
	g_pMatchFramework->GetEventsSubscription()->Subscribe( sink );
}

void PyMatchEventsSubscription::Unsubscribe( PyMatchEventsSink *sink )
{
	g_pMatchFramework->GetEventsSubscription()->Unsubscribe( sink );
}

void PyMatchEventsSubscription::BroadcastEvent( KeyValues *pEvent )
{
	return g_pMatchFramework->GetEventsSubscription()->BroadcastEvent( pEvent );
}

void PyMatchEventsSubscription::RegisterEventData( KeyValues *pEventData )
{
	return g_pMatchFramework->GetEventsSubscription()->RegisterEventData( pEventData );
}

KeyValues * PyMatchEventsSubscription::GetEventData( char const *szEventDataKey )
{
	return g_pMatchFramework->GetEventsSubscription()->GetEventData( szEventDataKey );
}

//=============================================================================
// MatchSession
//=============================================================================
KeyValues *PyMatchSession::GetSessionSystemData()
{
	MATCHSESSION_VALID;

	return g_pMatchFramework->GetMatchSession()->GetSessionSystemData();
}

KeyValues *PyMatchSession::GetSessionSettings()
{
	MATCHSESSION_VALID;
	return g_pMatchFramework->GetMatchSession()->GetSessionSettings();
}

void PyMatchSession::UpdateSessionSettings( KeyValues *pSettings )
{
	MATCHSESSION_VALID;
	g_pMatchFramework->GetMatchSession()->UpdateSessionSettings( pSettings );
}

void PyMatchSession::Command( KeyValues *pCommand )
{
	MATCHSESSION_VALID;
	g_pMatchFramework->GetMatchSession()->Command( pCommand );
}

//=============================================================================
// MatchSystem
//=============================================================================

bp::object PyMatchSystem::CreateGameSearchManager( KeyValues *pSettings )
{
	if( !pSettings )
		return bp::object();

	bp::object pysearchmanager = matchmaking.attr("SearchManager")();

	PySearchManager *pPyPlayerManager = bp::extract<PySearchManager *>( pysearchmanager );
	if( !pPyPlayerManager )
		return bp::object();

	ISearchManager *pSearchManager = g_pMatchFramework->GetMatchSystem()->CreateGameSearchManager( pSettings );
	if( !pSearchManager )
		return bp::object();

	pPyPlayerManager->SetSearchManagerInternal( pSearchManager, SrcPySystem()->CreateWeakRef( pysearchmanager ) );

	return pysearchmanager;
}

//=============================================================================
// SteamMatchmaking
//=============================================================================
void PySteamMatchmaking::AddRequestLobbyListDistanceFilter( ELobbyDistanceFilter eLobbyDistanceFilter )
{
	ISteamMatchmaking *pSteamMatchMaking = steamapicontext->SteamMatchmaking();
	if( pSteamMatchMaking )
	{
		pSteamMatchMaking->AddRequestLobbyListDistanceFilter( eLobbyDistanceFilter );
	}
}