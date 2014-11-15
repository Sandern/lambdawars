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

#endif // SRCPY_MATCHMAKING_H
