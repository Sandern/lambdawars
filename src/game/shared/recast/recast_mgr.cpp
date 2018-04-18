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
#include "wars/iwars_extension.h"

#ifndef CLIENT_DLL
#include "recast/recast_mapmesh.h"
#include "hl2wars_player.h"
#include "props.h"
#else
#include "c_hl2wars_player.h"
#endif // CLIENT_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
	ConVar recast_debug_mesh( "recast_debug_mesh", "human" );	
#endif // CLIENT_DLL

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
CRecastMgr::CRecastMgr() : m_bLoaded(false), m_Obstacles( 0, 0, DefLessFunc( EHANDLE ) )
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
void CRecastMgr::Init()
{
#ifndef CLIENT_DLL
	// Accessor for client debugging
	warsextension->SetRecastMgr( this );
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRecastMgr::Reset()
{
	m_bLoaded = false;

	m_Meshes.PurgeAndDeleteElements();

	m_Obstacles.Purge();

#ifndef CLIENT_DLL
	m_pendingPartialMeshUpdates.Purge();

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
static char* s_DefaultMeshNames[] = 
{
	"human",
	"medium",
	"large",
	"verylarge",
	"air"
};


bool CRecastMgr::InitDefaultMeshes()
{
	// Ensures default meshes exists, even if they don't have a mesh loaded.
	for( int i = 0; i < ARRAYSIZE( s_DefaultMeshNames ); i++ )
	{
		CRecastMesh *pMesh = GetMesh( s_DefaultMeshNames[i] );
		if( !pMesh )
		{
			pMesh = new CRecastMesh();
			pMesh->Init( s_DefaultMeshNames[i] );
			m_Meshes.Insert( pMesh->GetName(), pMesh );
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRecastMgr::InsertMesh( const char *name, float agentRadius, float agentHeight, float agentMaxClimb, float agentMaxSlope )
{
	CRecastMesh *pMesh = GetMesh( name );
	if( pMesh )
	{
		Warning( "CRecastMgr::InsertMesh: %s already exists!\n", name );
		return false;
	}

	pMesh = new CRecastMesh();
	pMesh->Init( name, agentRadius, agentHeight, agentMaxClimb, agentMaxSlope );
	m_Meshes.Insert( pMesh->GetName(), pMesh );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRecastMgr::Update( float dt )
{
	VPROF_BUDGET( "CRecastMgr::Update", "RecastNav" );

#ifndef CLIENT_DLL
	UpdateRebuildPartial();
#endif // CLIENT_DLL

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
// Purpose: Determines best nav mesh radius/height
//-----------------------------------------------------------------------------
CRecastMesh *CRecastMgr::FindBestMeshForRadiusHeight( float radius, float height )
{
	int bestIdx = -1;
	float fBestRadiusDiff = 0;
	float fBestHeightDiff = 0;
	for ( int i = m_Meshes.First(); i != m_Meshes.InvalidIndex(); i = m_Meshes.Next(i ) )
	{
		CRecastMesh *pMesh = m_Meshes[ i ];
		if( !pMesh->IsLoaded() )
		{
			continue;
		}

		// Only consider fitting meshes
		if( radius > pMesh->GetAgentRadius() || height > pMesh->GetAgentHeight() )
		{
			continue;
		}

		// From these meshes, pick the best fitting one
		float fRadiusDiff = fabs( pMesh->GetAgentRadius() - radius );
		float fHeightDiff = fabs( pMesh->GetAgentHeight() - height );
		if( bestIdx == -1 || (fRadiusDiff + fHeightDiff <= fBestRadiusDiff + fBestHeightDiff) )
		{
			bestIdx = i;
			fBestRadiusDiff = fRadiusDiff;
			fBestHeightDiff = fHeightDiff;
		}
	}
	return GetMesh( bestIdx );
}

//-----------------------------------------------------------------------------
// Purpose: Determines best nav mesh for entity
//-----------------------------------------------------------------------------
CRecastMesh *CRecastMgr::FindBestMeshForEntity( CBaseEntity *pEntity )
{
	if( !pEntity )
		return NULL;
	return FindBestMeshForRadiusHeight( pEntity->CollisionProp()->BoundingRadius2D(), pEntity->CollisionProp()->OBBSize().z );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CRecastMgr::FindMeshIndex( const char *name )
{
	return m_Meshes.Find( name );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRecastMgr::IsMeshLoaded( const char *name )
{
	CRecastMesh *pMesh = GetMesh( name );
	return pMesh != NULL && pMesh->IsLoaded();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CRecastMgr::FindBestMeshNameForRadiusHeight( float radius, float height )
{
	CRecastMesh *pMesh = FindBestMeshForRadiusHeight( radius, height );
	return pMesh ? pMesh->GetName() : NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CRecastMgr::FindBestMeshNameForEntity( CBaseEntity *pEntity )
{
	CRecastMesh *pMesh = FindBestMeshForEntity( pEntity );
	return pMesh ? pMesh->GetName() : NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
dtNavMesh* CRecastMgr::GetNavMesh( const char *meshName )
{
	int idx = FindMeshIndex( meshName );
	if( m_Meshes.IsValidIndex( idx ) )
	{
		return m_Meshes[idx]->GetNavMesh();
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
dtNavMeshQuery* CRecastMgr::GetNavMeshQuery( const char *meshName )
{
	int idx = FindMeshIndex( meshName );
	if( m_Meshes.IsValidIndex( idx ) )
	{
		return m_Meshes[idx]->GetNavMeshQuery();
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
IMapMesh* CRecastMgr::GetMapMesh()
{
#ifdef CLIENT_DLL
	return NULL;
#else
	return m_pMapMesh;
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: Adds an obstacle with radius and height for entity
//-----------------------------------------------------------------------------
bool CRecastMgr::AddEntRadiusObstacle( CBaseEntity *pEntity, float radius, float height )
{
	if( !pEntity )
		return false;

	NavObstacleArray_t &obstacle = FindOrCreateObstacle( pEntity );
	obstacle.areaId = DetermineAreaID( pEntity, -Vector( radius, radius, radius ), Vector( radius, radius, radius ) );

	bool bSuccess = true;
	for ( int i = m_Meshes.First(); i != m_Meshes.InvalidIndex(); i = m_Meshes.Next(i ) )
	{
		CRecastMesh *pMesh = m_Meshes[ i ];
		// TODO: better check. Needs to filter out obstacles for air mesh
		if( pMesh->GetAgentHeight() - 50.0f > height )
			continue;

		obstacle.obs.AddToTail();
		obstacle.obs.Tail().meshIndex = i;
		obstacle.obs.Tail().ref = pMesh->AddTempObstacle( pEntity->GetAbsOrigin(), radius + 1.0f, height, obstacle.areaId );
		if( obstacle.obs.Tail().ref == 0 )
			bSuccess = false;
	}
	return bSuccess;
}

//-----------------------------------------------------------------------------
// Purpose: Adds an obstacle based on bounds and height for entity
// TODO: 
//-----------------------------------------------------------------------------
bool CRecastMgr::AddEntBoxObstacle( CBaseEntity *pEntity, const Vector &mins, const Vector &maxs, float height )
{
	if( !pEntity )
		return false;

	matrix3x4_t transform; // model to world transformation
	AngleMatrix( QAngle(0, pEntity->GetAbsAngles()[YAW], 0), pEntity->GetAbsOrigin(), transform );

	Vector convexHull[4];

	NavObstacleArray_t &obstacle = FindOrCreateObstacle( pEntity );
	obstacle.areaId = DetermineAreaID( pEntity, mins, maxs );

	bool bSuccess = true;
	for ( int i = m_Meshes.First(); i != m_Meshes.InvalidIndex(); i = m_Meshes.Next(i ) )
	{
		CRecastMesh *pMesh = m_Meshes[ i ];
		// TODO: better check. Needs to filter out obstacles for air mesh
		if( pMesh->GetAgentHeight() - 50.0f > height )
			continue;

		float erodeDist = pMesh->GetAgentRadius() + 8.0f;

		VectorTransform( mins + Vector(-erodeDist, -erodeDist, 0), transform, convexHull[0] );
		VectorTransform( Vector(mins.x, maxs.y, mins.z) + Vector(-erodeDist, erodeDist, 0), transform, convexHull[1] );
		VectorTransform( Vector(maxs.x, maxs.y, mins.z) + Vector(erodeDist, erodeDist, 0), transform, convexHull[2] );
		VectorTransform( Vector(maxs.x, mins.y, mins.z) + Vector(erodeDist, -erodeDist, 0), transform, convexHull[3] );

		/*for( int j = 0; j < 4; j++ )
		{
			int next = (j+1) == 4 ? 0 : j + 1;
			NDebugOverlay::Line( convexHull[j], convexHull[next], 0, 255, 0, true, 10.0f );
		}*/

		obstacle.obs.AddToTail();
		obstacle.obs.Tail().meshIndex = i;
		obstacle.obs.Tail().ref = pMesh->AddTempObstacle( pEntity->GetAbsOrigin(), convexHull, 4, height, obstacle.areaId );

		if( obstacle.obs.Tail().ref == 0 )
			bSuccess = false;
	}
	return bSuccess;
}

//-----------------------------------------------------------------------------
// Purpose: Removes any obstacle associated with the entity
//-----------------------------------------------------------------------------
bool CRecastMgr::RemoveEntObstacles( CBaseEntity *pEntity )
{
	if( !pEntity )
		return false;

	bool bSuccess = true;
	int idx = m_Obstacles.Find( pEntity );
	if( m_Obstacles.IsValidIndex( idx ) )
	{
		for( int i = 0; i < m_Obstacles[idx].obs.Count(); i++ )
		{
			if( m_Meshes.IsValidIndex( m_Obstacles[idx].obs[i].meshIndex ) )
			{
				CRecastMesh *pMesh = m_Meshes[ m_Obstacles[idx].obs[i].meshIndex ];
				if( !pMesh->RemoveObstacle( m_Obstacles[idx].obs[i].ref ) )
					bSuccess = false;
			}
		}

		m_Obstacles.RemoveAt( idx );
		pEntity->SetNavObstacleRef( NAV_OBSTACLE_INVALID_INDEX );
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Determines the area id in such a way potential touching obstacles
//			don't have the same area id. There are about 16 available area ids.
//-----------------------------------------------------------------------------
unsigned char CRecastMgr::DetermineAreaID( CBaseEntity *pEntity, const Vector &mins, const Vector &maxs )
{
	unsigned char areaId = SAMPLE_POLYAREA_OBSTACLE_START;

	// Determine areaId
	bool usedPolyAreas[20];
	V_memset( usedPolyAreas, 0, ARRAYSIZE(usedPolyAreas) * sizeof(bool) );
	CBaseEntity *pEnts[256];
	int n = UTIL_EntitiesInBox( pEnts, 256, pEntity->GetAbsOrigin() + mins - Vector(64.0f, 64.0f, 64.0f), pEntity->GetAbsOrigin() + maxs + Vector(64.0f, 64.0f, 64.0f), 0 );
	for( int i = 0; i < n; i++ )
	{
		CBaseEntity *pEnt = pEnts[i];
		if( !pEnt || pEnt == pEntity || pEnt->GetNavObstacleRef() == NAV_OBSTACLE_INVALID_INDEX )
		{
			continue;
		}

		unsigned char otherAreaId = m_Obstacles.Element( pEnt->GetNavObstacleRef() ).areaId;

		// Sanity check, would indicate an uninitialized obstacle if this happens.
		if( otherAreaId < SAMPLE_POLYAREA_OBSTACLE_START || otherAreaId > SAMPLE_POLYAREA_OBSTACLE_END )
		{
			Warning( "Obstacle has valid ref, but invalid area id? %d\n", otherAreaId );
			continue;
		}
		usedPolyAreas[otherAreaId] = true;
	}

	while( areaId != SAMPLE_POLYAREA_OBSTACLE_END - 1 && usedPolyAreas[areaId] )
	{
		areaId++;
	}

	return areaId;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
NavObstacleArray_t &CRecastMgr::FindOrCreateObstacle( CBaseEntity *pEntity )
{
	int idx = m_Obstacles.Find( pEntity );
	if( !m_Obstacles.IsValidIndex( idx ) )
	{
		idx = m_Obstacles.Insert( pEntity );
		pEntity->SetNavObstacleRef( idx );
	}
	return m_Obstacles[idx];
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Saves the generated navigation meshes
//-----------------------------------------------------------------------------
void CRecastMgr::DebugRender()
{
	int idx = m_Meshes.Find( recast_debug_mesh.GetString() );
	if( idx == m_Meshes.InvalidIndex() )
	{
		// Might be visualizing a server mesh that does not exist on the client
		// Insert dummy mesh on the fly.
		IRecastMgr *pRecastMgr = warsextension->GetRecastMgr();
		if( pRecastMgr->GetNavMesh( recast_debug_mesh.GetString() ) )
		{
			CRecastMesh *pMesh = new CRecastMesh();
			pMesh->Init( recast_debug_mesh.GetString() );
			idx = m_Meshes.Insert( pMesh->GetName(), pMesh );
		}
	}

	if( m_Meshes.IsValidIndex( idx ) )
	{
		m_Meshes[idx]->DebugRender();
	}
}
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Prints list of navigation meshes
//-----------------------------------------------------------------------------
void CRecastMgr::DebugListMeshes()
{
	int idx = m_Meshes.First();
	while( m_Meshes.IsValidIndex( idx ) )
	{
		CRecastMesh *pMesh = m_Meshes.Element(idx);
		Msg( "%d: %s (agent radius: %f, height: %f, climb: %f, slope: %f, cell size: %f, cell height: %f)\n", idx, 
			pMesh->GetName(),
			pMesh->GetAgentRadius(), pMesh->GetAgentHeight(), pMesh->GetAgentMaxClimb(), pMesh->GetAgentMaxSlope(),
			pMesh->GetCellSize(), pMesh->GetCellHeight());
		idx = m_Meshes.Next( idx );
	}
}

#ifndef CLIENT_DLL
CON_COMMAND_F( recast_loadmapmesh, "", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
	s_RecastMgr.LoadMapMesh();
}

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
	s_RecastMgr.DebugListMeshes();
}

#ifndef CLIENT_DLL
CON_COMMAND_F( recast_readd_phys_props, "", FCVAR_CHEAT )
{
	for( CBaseEntity *pEntity = gEntList.FirstEnt(); pEntity != NULL; pEntity = gEntList.NextEnt( pEntity ) )
	{
		if( FClassnameIs( pEntity, "prop_physics" ) )
		{
			CBaseProp *pProp = dynamic_cast< CBaseProp * >( pEntity );
			if( pProp )
				pProp->UpdateNavObstacle( true );
		}
	}
}
#endif // CLIENT_DLL
