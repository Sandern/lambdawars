//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "srcpy.h"
#include "srcpy_matchmaking.h"
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

bool PyMKIsSessionActive()
{
	return g_pMatchFramework->GetMatchSession() != NULL;
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
