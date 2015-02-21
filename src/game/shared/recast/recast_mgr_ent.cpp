//========= Copyright, Sandern Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "recast_mgr_ent.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

BEGIN_DATADESC( CRecastMgrEnt )
END_DATADESC()

IMPLEMENT_NETWORKCLASS_ALIASED( RecastMgrEnt, DT_RecastMgrEnt );
BEGIN_NETWORK_TABLE( CRecastMgrEnt, DT_RecastMgrEnt )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_spawnflags ) ),
#else
	SendPropInt( SENDINFO(m_spawnflags), -1, SPROP_NOSCALE )
#endif // CLIENT_DLL
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( recast_mgr, CRecastMgrEnt );

static CRecastMgrEnt *s_pRecastMgrEnt = NULL;

CRecastMgrEnt *GetRecastMgrEnt()
{
	return s_pRecastMgrEnt;
}

CRecastMgrEnt::CRecastMgrEnt()
{
	s_pRecastMgrEnt = this;
}

CRecastMgrEnt::~CRecastMgrEnt()
{
	if( s_pRecastMgrEnt == this )
		s_pRecastMgrEnt = NULL;
}