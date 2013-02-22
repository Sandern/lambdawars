//====== Copyright © 2013 Sandern Corporation, All rights reserved. ===========//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "src_python_matchmaking.h"
#include "matchmaking/imatchframework.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Entry point to create session
void PyMKCreateSession( KeyValues *pSettings )
{
	g_pMatchFramework->CreateSession( pSettings );
}

// Entry point to match into a session
void PyMKMatchSession( KeyValues *pSettings )
{
	g_pMatchFramework->MatchSession( pSettings );
}

KeyValues *PyMatchSession::GetSessionSystemData()
{
	return g_pMatchFramework->GetMatchSession()->GetSessionSystemData();
}

KeyValues *PyMatchSession::GetSessionSettings()
{
	return g_pMatchFramework->GetMatchSession()->GetSessionSettings();
}

void PyMatchSession::UpdateSessionSettings( KeyValues *pSettings )
{
	g_pMatchFramework->GetMatchSession()->UpdateSessionSettings( pSettings );
}

void PyMatchSession::Command( KeyValues *pCommand )
{
	g_pMatchFramework->GetMatchSession()->Command( pCommand );
}