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
	// Clear any existing mesh
	Reset();

	// Create meshes
	CRecastMesh *pMesh = new CRecastMesh();
	pMesh->SetName( "soldier" );
	if( pMesh->Build() )
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
	s_RecastMgr.Build();
	s_RecastMgr.Save();
}

CON_COMMAND_F( recast_save, "", FCVAR_CHEAT )
{
	s_RecastMgr.Save();

	// Something like that:
	//engine->ClientCmd("recast_reload\n");
}
#endif // CLIENT_DLL

#ifndef CLIENT_DLL
CON_COMMAND_F( recast_listmeshes, "", FCVAR_CHEAT )
#else
CON_COMMAND_F( cl_recast_listmeshes, "", FCVAR_CHEAT )
#endif // CLIENT_DLL
{
	CUtlDict< CRecastMesh *, int > &meshes = s_RecastMgr.GetMeshes();
	int idx = meshes.First();
	while( meshes.IsValidIndex( idx ) )
	{
		Msg( "%d: %s\n", idx, meshes.Element(idx)->GetName() );
		idx = meshes.Next( idx );
	}
}
