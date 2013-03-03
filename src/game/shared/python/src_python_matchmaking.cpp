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

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
// Search Manager
//=============================================================================
PySearchManager::PySearchManager() : m_pSearchManager(NULL)
{

}

PySearchManager::~PySearchManager()
{
	if( m_pSearchManager )
	{
		m_pSearchManager->Destroy();
		m_pSearchManager = NULL;
	}
}

void PySearchManager::EnableResultsUpdate( bool bEnable, KeyValues *pSearchParams )
{
	m_pSearchManager->EnableResultsUpdate( bEnable, pSearchParams );
}

int PySearchManager::GetNumResults()
{
	return m_pSearchManager->GetNumResults();
}

void PySearchManager::SetSearchManagerInternal( ISearchManager *pSearchManager )
{
	m_pSearchManager = pSearchManager;
}

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

	bp::object playermgr = matchmaking.attr("SearchManager")();

	PySearchManager *pPyPlayerManager = bp::extract<PySearchManager *>( playermgr );
	if( !pPyPlayerManager )
		return bp::object();

	ISearchManager *pSearchManager = g_pMatchFramework->GetMatchSystem()->CreateGameSearchManager( pSettings );
	if( !pSearchManager )
		return bp::object();

	pPyPlayerManager->SetSearchManagerInternal( pSearchManager );

	return playermgr;
}