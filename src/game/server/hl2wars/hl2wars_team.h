//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Team management class. Contains all the details for a specific team
//
// $NoKeywords: $
//=============================================================================//

#ifndef HL2WARS_TEAM_H
#define HL2WARS_TEAM_H

#ifdef _WIN32
#pragma once
#endif


#include "utlvector.h"
#include "team.h"

//-----------------------------------------------------------------------------
// Purpose: Team Manager
//-----------------------------------------------------------------------------
class CHL2WarsTeam : public CTeam
{
	DECLARE_CLASS( CHL2WarsTeam, CTeam );
	DECLARE_SERVERCLASS();

public:

	// Initialization
	virtual void Init( const char *pName, int iNumber );
	const unsigned char *GetEncryptionKey( void ) { return g_pGameRules->GetEncryptionKey(); }

	virtual const char *GetTeamName( void ) { return "#Teamname_Spectators"; }

	void ResetScores( void );
};


extern CHL2WarsTeam *GetGlobalHL2WarsTeam( int iIndex );


#endif // HL2WARS_TEAM_H
