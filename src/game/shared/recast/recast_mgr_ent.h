//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:	
//
//=============================================================================//

#ifndef RECAST_MGR_ENT_H
#define RECAST_MGR_ENT_H

#ifdef _WIN32
#pragma once
#endif

#define SF_DISABLE_MESH_HUMAN				0x000001
#define SF_DISABLE_MESH_MEDIUM				0x000002
#define SF_DISABLE_MESH_LARGE				0x000004
#define SF_DISABLE_MESH_VERYLARGE			0x000008
#define SF_DISABLE_MESH_AIR					0x000010

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
#if defined( CLIENT_DLL )
#define CRecastMgrEnt C_RecastMgrEnt
#endif
class CRecastMgrEnt : public CBaseEntity
{
public:
	DECLARE_CLASS( CRecastMgrEnt, CBaseEntity );
	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS();

	CRecastMgrEnt();
	~CRecastMgrEnt();

#if defined( GAME_DLL )
	int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}
#endif
};

CRecastMgrEnt *GetRecastMgrEnt();

#endif // RECAST_MGR_ENT_H