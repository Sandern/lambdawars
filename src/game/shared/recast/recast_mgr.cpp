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
bool CRecastMgr::Load()
{
	CRecastMesh *pMesh = new CRecastMesh();
	pMesh->Load();
	m_Meshes.Insert( "soldier", pMesh );
	return true;
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
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Saves the generated navigation meshes
//-----------------------------------------------------------------------------
bool CRecastMgr::Save()
{
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

#ifdef CLIENT_DLL
CON_COMMAND_F( recast_rebuild, "", FCVAR_CHEAT )
#else
CON_COMMAND_F( recast_rebuild_server, "", FCVAR_CHEAT )
#endif // CLIENT_DLL
{
	s_RecastMgr.Load();

#ifdef CLIENT_DLL
	engine->ServerCmd("recast_rebuild_server\n");
#endif // CLIENT_DLL
}
