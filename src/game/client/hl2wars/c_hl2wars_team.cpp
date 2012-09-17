//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose: Client side C_HL2WarsTeam class
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "engine/IEngineSound.h"
#include "hud.h"
#include "recvproxy.h"
#include "c_hl2wars_team.h"

#include <vgui/ILocalize.h>
#include <tier3/tier3.h>
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//Tony; undefine what I did in the header so everything from this point forward functions correctly.
#undef CHL2WarsTeam

IMPLEMENT_CLIENTCLASS_DT(C_HL2WarsTeam, DT_HL2WarsTeam, CHL2WarsTeam)
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Get a pointer to the specified TF team manager
//-----------------------------------------------------------------------------
C_HL2WarsTeam *GetGlobalHL2WarsTeam( int iIndex )
{
	return (C_HL2WarsTeam*)GetGlobalTeam( iIndex );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_HL2WarsTeam::C_HL2WarsTeam()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_HL2WarsTeam::~C_HL2WarsTeam()
{
}
char *C_HL2WarsTeam::Get_Name( void )
{
	wchar_t *teamname;
	if (m_szTeamname[0] == '#')
	{
		teamname = g_pVGuiLocalize->Find(m_szTeamname);

		char ansi[128];
		g_pVGuiLocalize->ConvertUnicodeToANSI( teamname, ansi, sizeof( ansi ) );

		return strdup(ansi);
	}
	else 
		return m_szTeamname;
}


IMPLEMENT_CLIENTCLASS_DT(C_HL2WarsTeam_Unassigned, DT_HL2WarsTeam_Unassigned, CHL2WarsTeam_Unassigned)
END_RECV_TABLE()

C_HL2WarsTeam_Unassigned::C_HL2WarsTeam_Unassigned()
{
}
