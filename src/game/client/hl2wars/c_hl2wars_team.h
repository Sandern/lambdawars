//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose: Client side CTFTeam class
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_HL2WARS_TEAM_H
#define C_HL2WARS_TEAM_H
#ifdef _WIN32
#pragma once
#endif

#include "c_team.h"
#include "shareddefs.h"

class C_BaseEntity;
class C_BaseObject;
class CBaseTechnology;

//Tony; so we can call this from shared code!
#define CHL2WarsTeam C_HL2WarsTeam

//-----------------------------------------------------------------------------
// Purpose: TF's Team manager
//-----------------------------------------------------------------------------
class C_HL2WarsTeam : public C_Team
{
	DECLARE_CLASS( C_HL2WarsTeam, C_Team );
	DECLARE_CLIENTCLASS();

public:

					C_HL2WarsTeam();
	virtual			~C_HL2WarsTeam();

	virtual char	*Get_Name( void );
};

class C_HL2WarsTeam_Unassigned : public C_HL2WarsTeam
{
	DECLARE_CLASS( C_HL2WarsTeam_Unassigned, C_HL2WarsTeam );
public:
	DECLARE_CLIENTCLASS();

				     C_HL2WarsTeam_Unassigned();
	 virtual		~C_HL2WarsTeam_Unassigned() {}
};

extern C_HL2WarsTeam *GetGlobalHL2WarsTeam( int iIndex );

#endif // C_HL2WARS_TEAM_H
