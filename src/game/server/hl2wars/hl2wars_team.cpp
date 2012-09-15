//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Team management class. Contains all the details for a specific team
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "hl2wars_team.h"
#include "entitylist.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


// Datatable
IMPLEMENT_SERVERCLASS_ST(CHL2WarsTeam, DT_HL2WarsTeam)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( hl2wars_team_manager, CHL2WarsTeam );

//-----------------------------------------------------------------------------
// Purpose: Get a pointer to the specified TF team manager
//-----------------------------------------------------------------------------
CHL2WarsTeam *GetGlobalHL2WarsTeam( int iIndex )
{
	return (CHL2WarsTeam*)GetGlobalTeam( iIndex );
}


//-----------------------------------------------------------------------------
// Purpose: Needed because this is an entity, but should never be used
//-----------------------------------------------------------------------------
void CHL2WarsTeam::Init( const char *pName, int iNumber )
{
	BaseClass::Init( pName, iNumber );

	// Only detect changes every half-second.
	NetworkProp()->SetUpdateInterval( 0.75f );
}

void CHL2WarsTeam::ResetScores( void )
{
	SetRoundsWon(0);
	SetScore(0);
}

// UNASSIGNED
//==================

class CHL2WarsTeam_Unassigned : public CHL2WarsTeam
{
	DECLARE_CLASS( CHL2WarsTeam_Unassigned, CHL2WarsTeam );
	DECLARE_SERVERCLASS();

	virtual void Init( const char *pName, int iNumber )
	{
		BaseClass::Init( pName, iNumber );
	}

	virtual const char *GetTeamName( void ) { return "#Teamname_Unassigned"; }
};

IMPLEMENT_SERVERCLASS_ST(CHL2WarsTeam_Unassigned, DT_HL2WarsTeam_Unassigned)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( hl2wars_team_unassigned, CHL2WarsTeam_Unassigned );

