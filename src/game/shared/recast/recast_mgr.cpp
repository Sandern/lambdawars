//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// Note: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "recast/recast_mgr.h"
#include "recast/recast_mesh.h"

#ifndef CLIENT_DLL
#include "recast/recast_mapmesh.h"
#include "hl2wars_player.h"
#else
#include "c_hl2wars_player.h"
#endif // CLIENT_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Accessor
//-----------------------------------------------------------------------------
static CRecastMgr s_RecastMgr;
CRecastMgr &RecastMgr()
{
	return s_RecastMgr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRecastMgr::CRecastMgr()
{
#ifndef CLIENT_DLL
	m_pMapMesh = NULL;
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRecastMgr::~CRecastMgr()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRecastMgr::Reset()
{
	for ( int i = m_Meshes.First(); i != m_Meshes.InvalidIndex(); i = m_Meshes.Next(i ) )
	{
		CRecastMesh *pMesh = m_Meshes[ i ];
		pMesh->Reset();
	}
	m_Meshes.PurgeAndDeleteElements();

#ifndef CLIENT_DLL
	if( m_pMapMesh )
	{
		delete m_pMapMesh;
		m_pMapMesh = NULL;
	}
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRecastMgr::Update( float dt )
{
	VPROF_BUDGET( "CRecastMgr::Update", "RecastNav" );

	for ( int i = m_Meshes.First(); i != m_Meshes.InvalidIndex(); i = m_Meshes.Next(i ) )
	{
		CRecastMesh *pMesh = m_Meshes[ i ];
		pMesh->Update( dt );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRecastMesh *CRecastMgr::GetMesh( int index )
{
	if( !m_Meshes.IsValidIndex( index ) )
	{
		return NULL;
	}
	return m_Meshes[index];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CRecastMgr::FindMeshIndex( const char *name )
{
	return m_Meshes.Find( name );
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Builds all navigation meshes
//-----------------------------------------------------------------------------
bool CRecastMgr::Build()
{
	// Clear any existing mesh
	Reset();

	// Load map mesh
	CMapMesh *pMapMesh = new CMapMesh();
	if( !pMapMesh->Load() )
	{
		Warning("CRecastMesh::Load: failed to load map data!\n");
		return false;
	}

	// Create meshes
	CRecastMesh *pMesh = new CRecastMesh();
	pMesh->SetName( "soldier" );
	if( pMesh->Build( pMapMesh ) )
	{
		m_Meshes.Insert( pMesh->GetName(), pMesh );
	}

	return true;
}
#endif // CLIENT_DLL

#ifdef CLIENT_DLL
ConVar recast_debug_mesh("recast_debug_mesh", "soldier");
//-----------------------------------------------------------------------------
// Purpose: Saves the generated navigation meshes
//-----------------------------------------------------------------------------
void CRecastMgr::DebugRender()
{
	int idx = m_Meshes.Find( recast_debug_mesh.GetString() );
	if( m_Meshes.IsValidIndex( idx ) )
	{
		m_Meshes[idx]->DebugRender();
	}
}
#endif // CLIENT_DLL

#ifndef CLIENT_DLL
CON_COMMAND_F( recast_build, "", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
	s_RecastMgr.Build();
	s_RecastMgr.Save();

	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if( pPlayer )
	{
		engine->ClientCommand( pPlayer->edict(), "cl_recast_reload\n" );
	}
}

CON_COMMAND_F( recast_save, "", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
	s_RecastMgr.Save();

	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if( pPlayer )
	{
		engine->ClientCommand( pPlayer->edict(), "cl_recast_reload\n" );
	}
}
#endif // CLIENT_DLL

#ifndef CLIENT_DLL
CON_COMMAND_F( recast_reload, "Reload the Recast Navigation Mesh from disk on server", FCVAR_CHEAT )
#else
CON_COMMAND_F( cl_recast_reload, "Reload the Recast Navigation Mesh from disk on client", FCVAR_CHEAT )
#endif // CLIENT_DLL
{
#ifndef CLIENT_DLL
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif // CLIENT_DLL
	s_RecastMgr.Load();
}

#ifndef CLIENT_DLL
CON_COMMAND_F( recast_listmeshes, "", FCVAR_CHEAT )
#else
CON_COMMAND_F( cl_recast_listmeshes, "", FCVAR_CHEAT )
#endif // CLIENT_DLL
{
#ifndef CLIENT_DLL
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif // CLIENT_DLL
	CUtlDict< CRecastMesh *, int > &meshes = s_RecastMgr.GetMeshes();
	int idx = meshes.First();
	while( meshes.IsValidIndex( idx ) )
	{
		Msg( "%d: %s\n", idx, meshes.Element(idx)->GetName() );
		idx = meshes.Next( idx );
	}
}


#ifndef CLIENT_DLL
CON_COMMAND_F( recast_addobstacle, "", FCVAR_CHEAT )
#else
CON_COMMAND_F( cl_recast_addobstacle, "", FCVAR_CHEAT )
#endif // CLIENT_DLL
{
#ifndef CLIENT_DLL
	CHL2WarsPlayer *pPlayer = dynamic_cast<CHL2WarsPlayer *>( UTIL_GetCommandClient() );
#else
	C_HL2WarsPlayer *pPlayer = C_HL2WarsPlayer::GetLocalHL2WarsPlayer();
#endif // CLIENT_DLL
	if( !pPlayer )
	{
		return;
	}

	CUtlDict< CRecastMesh *, int > &meshes = s_RecastMgr.GetMeshes();
	for ( int i = meshes.First(); i != meshes.InvalidIndex(); i = meshes.Next(i ) )
	{
		CRecastMesh *pMesh = meshes[ i ];
		float radius = args.ArgC() > 1 ? atof( args[1] ) : 64.0f;
		float height = args.ArgC() > 2 ? atof( args[2] ) : 96.0f;
		pMesh->AddTempObstacle( pPlayer->GetMouseData().m_vEndPos, radius, height );
	}

#ifndef CLIENT_DLL
	engine->ClientCommand( pPlayer->edict(), "cl_recast_addobstacle %s %s\n", args[1], args[2] );
#endif // CLIENT_DLL
}