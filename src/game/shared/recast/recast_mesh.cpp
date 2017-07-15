//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// Note: Recasts expects "y" to be up, so y and z must be swapped everywhere. 
//
// TODO: 
// - Merge common code FindPath and FindPathDistance
// - Fix case with finding the shortest path to neutral obstacle (might try to go from other side)
// - Cache paths computed for unit type in same frame?
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "recast/recast_mesh.h"
#include "recast/recast_mgr.h"

#ifndef CLIENT_DLL
#include "recast/recast_mapmesh.h"
#else
#include "recast/recast_imapmesh.h"
#include "recast/recast_recastdebugdraw.h"
#include "recast/recast_debugdrawmesh.h"
#include "recast/recast_detourdebugdraw.h"
#include "wars/iwars_extension.h"
#include "recast_imgr.h"
#endif // CLIENT_DLL

#include "recast/recast_tilecache_helpers.h"
#include "recast/recast_common.h"

#include "detour/DetourNavMesh.h"
#include "detour/DetourNavMeshBuilder.h"
#include "detour/DetourCommon.h"

#ifndef CLIENT_DLL
#include "unit_navigator.h"
#endif // CLIENT_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef CLIENT_DLL
ConVar recast_findpath_debug( "recast_findpath_debug", "0", FCVAR_CHEAT, "" );
ConVar recast_areaslope_debug( "recast_areaslope_debug", "0", FCVAR_CHEAT, "" );
#else
ConVar recast_findpath_debug( "cl_recast_findpath_debug", "0", FCVAR_CHEAT, "" );
ConVar recast_areaslope_debug( "cl_recast_areaslope_debug", "0", FCVAR_CHEAT, "" );
#endif // CLIENT_DLL

// Defaults
static ConVar recast_maxslope( "recast_maxslope", "45.0", FCVAR_REPLICATED|FCVAR_CHEAT );
static ConVar recast_findpath_use_caching( "recast_findpath_use_caching", "1", FCVAR_REPLICATED|FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRecastMesh::CRecastMesh() :
	m_navMesh(0),
	m_tileCache(0),
	m_cacheBuildTimeMs(0),
	m_cacheCompressedSize(0),
	m_cacheRawSize(0),
	m_cacheLayerCount(0),
	m_cacheBuildMemUsage(0),
	m_navQuery(0),
	m_navQueryLimitedNodes(0),
	m_talloc(0),
	m_tcomp(0),
	m_tmproc(0)
{
	m_Name.Set("default");

	m_regionMinSize = 8;
	m_regionMergeSize = 20;
	m_edgeMaxLen = 12000.0f;
	m_edgeMaxError = 1.3f;
	m_vertsPerPoly = 6.0f;
	m_detailSampleDist = 600.0f;
	m_detailSampleMaxError = 100.0f;
	m_partitionType = SAMPLE_PARTITION_WATERSHED;

	m_maxTiles = 0;
	m_maxPolysPerTile = 0;
	m_tileSize = 48;

	m_allObstacleFlags = 0;
	unsigned short curFlag = SAMPLE_POLYFLAGS_OBSTACLE_START;
	while( curFlag != SAMPLE_POLYFLAGS_OBSTACLE_END )
	{
		m_allObstacleFlags |= curFlag;
		curFlag = curFlag << 1;
	}

	m_defaultFilter.setIncludeFlags( SAMPLE_POLYFLAGS_ALL ^ (SAMPLE_POLYFLAGS_DISABLED|m_allObstacleFlags) );
	m_defaultFilter.setExcludeFlags( 0 );

	m_obstacleFilter.setIncludeFlags( m_allObstacleFlags );
	m_obstacleFilter.setExcludeFlags( 0 );

	m_allFilter.setIncludeFlags( SAMPLE_POLYFLAGS_ALL ^ SAMPLE_POLYFLAGS_DISABLED );
	m_allFilter.setExcludeFlags( 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRecastMesh::~CRecastMesh()
{
	Reset();

	if( m_navQuery )
	{
		dtFreeNavMeshQuery( m_navQuery );
		m_navQuery = 0;
	}

	if( m_navQueryLimitedNodes )
	{
		dtFreeNavMeshQuery( m_navQueryLimitedNodes );
		m_navQueryLimitedNodes = 0;
	}

	if( m_talloc )
	{
		delete m_talloc;
		m_talloc = 0;
	}

	if( m_tcomp )
	{
		delete m_tcomp;
		m_tcomp = 0;
	}

	if( m_tmproc )
	{
		delete m_tmproc;
		m_tmproc = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRecastMesh::ComputeMeshSettings( const char *name, 
	float &fAgentRadius, float &fAgentHeight, float &fAgentMaxClimb, float &fAgentMaxSlope,
	float &fCellSize, float &fCellHeight, float &fTileSize )
{
	// Per type
	fAgentMaxSlope = recast_maxslope.GetFloat(); // Default slope for units
	fTileSize = 48;
	if( V_strncmp( name, "human", V_strlen( name ) ) == 0 )
	{
		// HULL_HUMAN, e.g. Soldier/human
		fAgentHeight = 72.0f;
		fAgentRadius = 18.5f;
		fAgentMaxClimb = 18.0f;
		fCellSize = round( fAgentRadius / 3.0f );
		fCellHeight = round( fCellSize / 2.0f );
		// Increase tile size a bit, so it's more or less the world size of the other types
		// Take care not to make this value to big, or tiles may not generate (some limit in tilecache somewhere?)
		fTileSize = 64;
	}
	else if( V_strncmp( name, "medium", V_strlen( name ) )== 0 )
	{
		// HULL_MEDIUM & HULL_TALL, e.g. antlion, hunter, dog
		fAgentHeight = 80.0f;
		fAgentRadius = 26.0f;
		fAgentMaxClimb = 18.0f;
		fCellSize = round( fAgentRadius / 3.0f );
		fCellHeight = round( fCellSize / 2.0f );
	}
	else if( V_strncmp( name, "large", V_strlen( name ) )== 0 )
	{
		// HULL_LARGE, e.g. Antlion Guard
		fAgentHeight = 100.0f;
		fAgentRadius = 58.0f; 
		fAgentMaxClimb = 18.0f;
		fCellSize = 10.0f;
		fCellHeight = round( fCellSize / 2.0f );
	}
	else if( V_strncmp( name, "verylarge", V_strlen( name ) )== 0 )
	{
		// HULL_LARGE, e.g. Antlion Guard Boss
		fAgentHeight = 150.0f;
		fAgentRadius = 90.0f; 
		fAgentMaxClimb = 18.0f;
		fCellSize = 12.0f;
		fCellHeight = round( fCellSize / 2.0f );
	}
	else if( V_strncmp( name, "air", V_strlen( name ) )== 0 )
	{
		// HULL_LARGE_CENTERED, e.g. Strider. Should also be good for gunship/helicop.
		fAgentHeight = 450.0f; // Not really that height ever, but don't want to have areas indoor
		fAgentRadius = 42.0f; 
		fAgentMaxClimb =  225.0f;
		fAgentMaxSlope = 70.0f;
		fCellSize = 10.0f;
		fCellHeight = round( fCellSize / 2.0f );
	}
	else
	{
		Warning( "CRecastMesh::ComputeMeshSettings: Unknown mesh %s\n", name );

		fAgentHeight = 72.0f; // => Soldier/human
		fAgentRadius = 18.5f; // => Soldier/human
		fAgentMaxClimb = 18.0f;
		fCellSize = 10.0f;
		fCellHeight = round( fCellSize / 2.0f );

		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRecastMesh::SharedInit(  const char *name )
{
	m_Name.Set( name );

	m_navQuery = dtAllocNavMeshQuery();
	m_navQueryLimitedNodes = dtAllocNavMeshQuery();

	m_talloc = new LinearAllocator( 96000 );
	m_tcomp = new FastLZCompressor;
	m_tmproc = new MeshProcess( name );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRecastMesh::Init( const char *name )
{
	SharedInit( name );

	// Shared settings by all
	ComputeMeshSettings( name, m_agentRadius, m_agentHeight, m_agentMaxClimb, m_agentMaxSlope,
		m_cellSize, m_cellHeight, m_tileSize );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRecastMesh::Init( const char *name, float agentRadius, float agentHeight, float agentMaxClimb, float agentMaxSlope )
{
	SharedInit( name );

	m_agentHeight = agentHeight;
	m_agentRadius = agentRadius;
	m_agentMaxClimb = agentMaxClimb;
	m_agentMaxSlope = agentMaxSlope;
	m_cellSize = round( m_agentRadius / 3.0f );
	m_cellHeight = round( m_cellSize / 2.0f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRecastMesh::PostLoad()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRecastMesh::Update( float dt )
{
	if( !IsLoaded() )
	{
		return;
	}

	// Will update obstacles, invalidates the last found path
	m_pathfindData.cacheValid = false;

	dtStatus status = m_tileCache->update( dt, m_navMesh );
	if( !dtStatusSucceed( status ) )
	{
		Warning("CRecastMesh::Update failed: \n");
		if( status & DT_OUT_OF_MEMORY )
			Warning("\tOut of memory. Consider increasing LinearAllocator buffer size.\n");
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRecastMesh::Reset()
{
	// Cleanup Nav mesh data
	if( m_navMesh )
	{
		dtFreeNavMesh(m_navMesh);
		m_navMesh = 0;
	}

	if( m_tileCache )
	{
		dtFreeTileCache(m_tileCache);
		m_tileCache = 0;
	}

	return true;
}

#ifdef CLIENT_DLL
ConVar recast_draw_trimeshslope("recast_draw_trimeshslope", "0", FCVAR_CHEAT, "" );
ConVar recast_draw_navmesh("recast_draw_navmesh", "0", FCVAR_CHEAT, "" );
ConVar recast_draw_server("recast_draw_server", "1", FCVAR_CHEAT, "" );

//-----------------------------------------------------------------------------
// Purpose: Draws the mesh
//-----------------------------------------------------------------------------
void CRecastMesh::DebugRender()
{
	DebugDrawMesh dd;

	if( recast_draw_trimeshslope.GetBool() )
	{
		const float texScale = 1.0f / (m_cellSize * 10.0f);

		IRecastMgr *pRecastMgr = warsextension->GetRecastMgr();
		if( pRecastMgr )
		{
			IMapMesh *pMapMesh = pRecastMgr->GetMapMesh();
			if( pMapMesh && pMapMesh->GetNorms() )
			{
				duDebugDrawTriMeshSlope(&dd, pMapMesh->GetVerts(), pMapMesh->GetNumVerts(),
										pMapMesh->GetTris(), pMapMesh->GetNorms(), pMapMesh->GetNumTris(),
										m_agentMaxSlope, texScale);
			}
		}
	}

	if( recast_draw_navmesh.GetBool() )
	{
		dtNavMesh *navMesh = NULL;
		dtNavMeshQuery *navQuery = NULL;
		if( recast_draw_server.GetBool() )
		{
			IRecastMgr *pRecastMgr = warsextension->GetRecastMgr();
			navMesh = pRecastMgr->GetNavMesh( m_Name.Get() );
			navQuery = pRecastMgr->GetNavMeshQuery( m_Name.Get() );
		}
		else
		{
			navMesh = m_navMesh;
			navQuery = m_navQuery;
		}

		if( navMesh != NULL && navQuery != NULL )
		{
			char m_navMeshDrawFlags = DU_DRAWNAVMESH_OFFMESHCONS|DU_DRAWNAVMESH_CLOSEDLIST;
			duDebugDrawNavMeshWithClosedList(&dd, *navMesh, *navQuery, m_navMeshDrawFlags);
		}
	}
}
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CRecastMesh::GetPolyRef( const Vector &vPoint, float fBeneathLimit, float fExtent2D )
{
	if( !IsLoaded() )
		return 0;

	float pos[3];
	pos[0] = vPoint[0];
	pos[1] = vPoint[2];
	pos[2] = vPoint[1];

	// The search distance along each axis. [(x, y, z)]
	float polyPickExt[3];
	polyPickExt[0] = fExtent2D;
	polyPickExt[1] = fBeneathLimit;
	polyPickExt[2] = fExtent2D;

	dtPolyRef ref;
	dtStatus status = m_navQuery->findNearestPoly(pos, polyPickExt, &m_defaultFilter, &ref, 0);
	if( !dtStatusSucceed( status ) )
	{
		return 0;
	}
	return ref;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRecastMesh::IsValidPolyRef( int polyRef )
{
	if( !IsLoaded() )
		return false;

	return polyRef >= 0 && m_navQuery->isValidPolyRef( (dtPolyRef)polyRef, &m_defaultFilter );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CRecastMesh::ClosestPointOnMesh( const Vector &vPoint, float fBeneathLimit, float fRadius )
{
	if( !IsLoaded() )
		return vec3_origin;

	float pos[3];
	float center[3];
	pos[0] = vPoint[0];
	pos[1] = vPoint[2];
	pos[2] = vPoint[1];

	center[0] = pos[0];
	center[1] = pos[1] - (fBeneathLimit / 2.0f) + 32.0f;
	center[2] = pos[2];

	// The search distance along each axis. [(x, y, z)]
	float polyPickExt[3];
	polyPickExt[0] = fRadius;
	polyPickExt[1] = (fBeneathLimit + 32.0f) / 2.0f;
	polyPickExt[2] = fRadius;

	dtPolyRef closestRef;
	dtStatus status = m_navQuery->findNearestPoly(center, polyPickExt, &m_defaultFilter, &closestRef, 0, pos);
	if( !dtStatusSucceed( status ) )
	{
		return vec3_origin;
	}

	float closest[3];
	status = m_navQuery->closestPointOnPoly(closestRef, pos, closest, NULL/*, &posOverPoly*/);
	if( !dtStatusSucceed( status ) )
	{
		return vec3_origin;
	}

	return Vector( closest[0], closest[2], closest[1] );
}

// Returns a random number [0..1)
static float frand()
{
//	return ((float)(rand() & 0xffff)/(float)0xffff);
//	return (float)rand()/(float)RAND_MAX;
	return random->RandomFloat();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CRecastMesh::RandomPointWithRadius( const Vector &vCenter, float fRadius, const Vector *pStartPoint )
{
	if( !IsLoaded() )
		return vec3_origin;

	float pos[3];
	float center[3];
	center[0] = vCenter[0];
	center[1] = vCenter[2];
	center[2] = vCenter[1];

	if( pStartPoint )
	{
		pos[0] = (*pStartPoint)[0];
		pos[1] = (*pStartPoint)[2];
		pos[2] = (*pStartPoint)[1];
	}
	else
	{
		dtVcopy(pos, center);
	}

	// The search distance along each axis. [(x, y, z)]
	float polyPickExt[3];
	polyPickExt[0] = 256.0f;
	polyPickExt[1] = MAX_COORD_FLOAT;
	polyPickExt[2] = 256.0f;

	dtPolyRef closestRef;
	dtStatus status = m_navQuery->findNearestPoly( pos, polyPickExt, &m_defaultFilter, &closestRef, 0 );
	if( !dtStatusSucceed( status ) )
	{
		return vec3_origin;
	}

	dtPolyRef eRef;
	float epos[3];
	status = m_navQuery->findRandomPointAroundCircle( closestRef, center, fRadius, &m_defaultFilter, frand, &eRef, epos );
	if( !dtStatusSucceed( status ) )
	{
		return vec3_origin;
	}

	return Vector( epos[0], epos[2], epos[1] );
}

static void calcTriNormal(const float* v0, const float* v1, const float* v2, float* norm)
{
	float e0[3], e1[3];
	rcVsub(e0, v1, v0);
	rcVsub(e1, v2, v0);
	rcVcross(norm, e0, e1);
	rcVnormalize(norm);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CRecastMesh::IsAreaFlat( const Vector &vCenter, const Vector &vExtents, float fSlope )
{
	dtStatus status;

	float center[3];
	center[0] = vCenter.x;
	center[1] = vCenter.z;
	center[2] = vCenter.y;

	float extents[3];
	extents[0] = vExtents.x;
	extents[1] = vExtents.z;
	extents[2] = vExtents.y;

	dtPolyRef polys[RECASTMESH_MAX_POLYS];
	int npolys = 0;

	status = m_navQuery->queryPolygons( center, extents, &m_defaultFilter, polys, &npolys, RECASTMESH_MAX_POLYS );
	if( !dtStatusSucceed( status ) )
	{
		return 0;
	}

	float norm[3];
	const dtMeshTile* tile = 0;
	const dtPoly* poly = 0;

	for( int i = 0; i < npolys; i++ )
	{
		// Get poly and tile.
		// The API input has been cheked already, skip checking internal data.
		m_navMesh->getTileAndPolyByRefUnsafe( polys[i], &tile, &poly );

		if (poly->getType() == DT_POLYTYPE_GROUND)
		{
			for( int j = 2; j < poly->vertCount; ++j )
			{
				const float* va = &tile->verts[poly->verts[0]*3];
				const float* vb = &tile->verts[poly->verts[j-1]*3];
				const float* vc = &tile->verts[poly->verts[j]*3];

				calcTriNormal( va, vb, vc, norm);

				if( recast_areaslope_debug.GetBool() )
				{
					Vector vPoint( (va[0] + vb[0] + vc[0]) / 3.0f, (va[2] + vb[2] + vc[2]) / 3.0f, (va[1] + vb[1] + vc[1]) / 3.0f );
					char buf[256];
					V_snprintf( buf, sizeof( buf ), "%f", norm[1] );
					NDebugOverlay::Text( vPoint, buf, false, recast_areaslope_debug.GetFloat() );
				}

				if( norm[1] < fSlope )
					return false;
			}
		}
	}

	return true;
}

typedef struct originalPolyFlags_t {
	dtPolyRef ref;
	unsigned short flags;
} originalPolyFlags_t;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void markObstaclePolygonsWalkableNavmesh(dtNavMesh* nav, NavmeshFlags* flags, dtPolyRef start, unsigned short obstacleFlag, CUtlVector< originalPolyFlags_t > &enabledPolys)
{
	dtStatus status;

	// If already visited, skip.
	if (flags->getFlags(start))
		return;
		
	PolyRefArray openList;
	openList.push(start);

	// Mark as visited, so start is not revisited again
	flags->setFlags(start, 1);

	while (openList.size())
	{
		const dtPolyRef ref = openList.pop();

		unsigned short polyFlags = 0;
		status = nav->getPolyFlags( ref, &polyFlags );
		if( !dtStatusSucceed( status ) )
		{
			continue;
		}

		// Only mark the polygons of the obstacle
		if( (polyFlags & obstacleFlag) == 0 )
		{
			continue;
		}

#if defined(_DEBUG)
		for( int k = 0; k < enabledPolys.Count(); k++ )
		{
			AssertMsg( enabledPolys[k].ref != ref, ("Already visited poly!")  );
		}
#endif // _DEBUG

		// Mark walkable and remember
		enabledPolys.AddToTail();
		enabledPolys.Tail().ref = ref;
		enabledPolys.Tail().flags = polyFlags;

		nav->setPolyFlags( ref, (polyFlags | SAMPLE_POLYFLAGS_WALK) );

		// Get current poly and tile.
		// The API input has been cheked already, skip checking internal data.
		const dtMeshTile* tile = 0;
		const dtPoly* poly = 0;
		nav->getTileAndPolyByRefUnsafe(ref, &tile, &poly);

		// Visit linked polygons.
		for (unsigned int i = poly->firstLink; i != DT_NULL_LINK; i = tile->links[i].next)
		{
			const dtPolyRef neiRef = tile->links[i].ref;
			// Skip invalid and already visited.
			if (!neiRef || flags->getFlags(neiRef))
				continue;
			// Mark as visited
			flags->setFlags(neiRef, 1);
			// Visit neighbours
			openList.push(neiRef);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
dtStatus CRecastMesh::DoFindPath( dtNavMeshQuery *navQuery, dtPolyRef startRef, dtPolyRef endRef, 
	float spos[3], float epos[3], bool bHasTargetAndIsObstacle, pathfind_resultdata_t &findpathData )
{
	VPROF_BUDGET( "CRecastMesh::DoFindPath", "RecastNav" );

	dtStatus status;

	if( recast_findpath_debug.GetBool() )
	{
		NDebugOverlay::Box( Vector(spos[0], spos[2], spos[1]), -Vector(8, 8, 8), Vector(8, 8, 8), 0, 255, 0, 255, 5.0f);
		NDebugOverlay::Box( Vector(epos[0], epos[2], epos[1]), -Vector(8, 8, 8), Vector(8, 8, 8), 0, 0, 255, 255, 5.0f);
	}

	if( !CanUseCachedPath( startRef, endRef, findpathData ) )
	{
		findpathData.cacheValid = false;

		CUtlVector< originalPolyFlags_t > enabledPolys;

		if( bHasTargetAndIsObstacle )
		{
			unsigned short obstacleFlag = 0;
			status = m_navMesh->getPolyFlags( endRef, &obstacleFlag );
			if( dtStatusSucceed( status ) )
			{
				NavmeshFlags *pNavMeshFlags = new NavmeshFlags;
				pNavMeshFlags->init( m_navMesh );

				// Find the obstacle flag
				obstacleFlag &= m_allObstacleFlags;
				// Make this ref and all linked refs with this flag walkable
				markObstaclePolygonsWalkableNavmesh( m_navMesh, pNavMeshFlags, endRef, obstacleFlag, enabledPolys );

				delete pNavMeshFlags;
			}
		}

		status = navQuery->findPath( startRef, endRef, spos, epos, &m_defaultFilter, findpathData.polys, &findpathData.npolys, RECASTMESH_MAX_POLYS );

		// Restore obstacle polyflags again (if any)
		for( int i = 0; i < enabledPolys.Count(); i++ )
		{
			m_navMesh->setPolyFlags( enabledPolys[i].ref, enabledPolys[i].flags );
		}

		findpathData.isPartial = (status & DT_PARTIAL_RESULT) != 0;

		if( recast_findpath_debug.GetBool() )
		{
			if( findpathData.isPartial )
				Warning( "Found a partial path to goal\n" );

			if( status & DT_OUT_OF_NODES )
				Warning( "Ran out of nodes during path find\n" );

			if( status & DT_BUFFER_TOO_SMALL )
				Warning( "Buffer is too small to hold path find result\n" );
		}

		if( !dtStatusSucceed( status ) )
		{
			return status;
		}
	}

	// Store information for caching purposes
	findpathData.cacheValid = true;
	findpathData.startRef = startRef;
	findpathData.endRef = endRef;

	if( findpathData.npolys )
	{	
		status = navQuery->findStraightPath(spos, epos, findpathData.polys, findpathData.npolys,
										findpathData.straightPath, findpathData.straightPathFlags,
										findpathData.straightPathPolys, &findpathData.nstraightPath, 
										RECASTMESH_MAX_POLYS, findpathData.straightPathOptions);
		return status;
	}

	return DT_FAILURE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
dtStatus CRecastMesh::ComputeAdjustedStartAndEnd( dtNavMeshQuery *navQuery, float spos[3], float epos[3], dtPolyRef &startRef, dtPolyRef &endRef, 
	float fBeneathLimit, bool bHasTargetAndIsObstacle, bool bDisallowBigPicker, const Vector *pStartTestPos )
{
	VPROF_BUDGET( "CRecastMesh::ComputeAdjustedStartAndEnd", "RecastNav" );

	dtStatus status;

	// The search distance along each axis. [(x, y, z)]
	float polyPickExt[3];
	polyPickExt[0] = 32.0f;
	polyPickExt[1] = fBeneathLimit;
	polyPickExt[2] = 32.0f;

	float polyPickExtEndBig[3];
	polyPickExtEndBig[0] = 512.0f;
	polyPickExtEndBig[1] = fBeneathLimit;
	polyPickExtEndBig[2] = 512.0f;

	// Find the start area. Optional use a different test position for finding the best area.
	// Don't care for now if start ref is an obstacle area or not
	if( pStartTestPos )
	{
		float spostest[3];
		spostest[0] = (*pStartTestPos)[0];
		spostest[1] = (*pStartTestPos)[2];
		spostest[2] = (*pStartTestPos)[1];
		status = navQuery->findNearestPoly(spos, polyPickExt, &m_defaultFilter, &startRef, 0, spostest);
	}
	else
	{
		status = navQuery->findNearestPoly(spos, polyPickExt, &m_defaultFilter, &startRef, 0);
	}

	if( !dtStatusSucceed( status ) )
	{
		return status;
	}

	// Try any filter next time
	if( !startRef )
	{
		if( pStartTestPos )
		{
			float spostest[3];
			spostest[0] = (*pStartTestPos)[0];
			spostest[1] = (*pStartTestPos)[2];
			spostest[2] = (*pStartTestPos)[1];
			status = navQuery->findNearestPoly(spos, polyPickExt, &m_allFilter, &startRef, 0, spostest);
		}
		else
		{
			status = navQuery->findNearestPoly(spos, polyPickExt, &m_allFilter, &startRef, 0);
		}
	}
	
	if( !dtStatusSucceed( status ) )
	{
		return status;
	}

	// Determine end point and area
	// Note: Special case for targets which act as obstacle on the nav mesh. These have a special flag to identify their polygon. We temporary mark
	// these polygons of the obstacle as walkable. Obstacles next to this obstacle have different flags, so they all have their own polygons.

	// Find the end area
	status = navQuery->findNearestPoly(epos, polyPickExt, bHasTargetAndIsObstacle ? &m_obstacleFilter : &m_defaultFilter, &endRef, 0);
	if( !dtStatusSucceed( status ) )
	{
		return status;
	}

	if( !endRef && !bDisallowBigPicker )
	{
		// Try again, bigger picker querying more tiles
		// This is mostly the case at the map borders, where we have no nav polygons. It's useful to have some tolerance, rather than just bug
		// out with a "can't move there" notification.
		status = navQuery->findNearestPoly(epos, polyPickExtEndBig, bHasTargetAndIsObstacle ? &m_obstacleFilter : &m_defaultFilter, &endRef, 0);
		if( !dtStatusSucceed( status ) )
		{
			return status;
		}
	}

	if( !endRef && bHasTargetAndIsObstacle )
	{
		// Work-around: in case of some small physics obstacles may not generate as an obstacle on the mesh
		// Todo: figure out something better?
		status = navQuery->findNearestPoly(epos, polyPickExt, &m_defaultFilter, &endRef, 0);
		if( !dtStatusSucceed( status ) )
		{
			return status;
		}
	}

	if( endRef )
	{
		float epos2[3];
		dtVcopy(epos2, epos);
		status = navQuery->closestPointOnPoly(endRef, epos2, epos, 0);
	}
	return status;
}

//-----------------------------------------------------------------------------
// Purpose: Checks if we can use the previous computed path.
//			This may not always give the correct result, because due the start
//			and end positions it may have computed a different path. But this is
//			good enough, because it will mostly be triggered for selection of
//			units.
//-----------------------------------------------------------------------------
bool CRecastMesh::CanUseCachedPath( dtPolyRef startRef, dtPolyRef endRef, pathfind_resultdata_t &pathResultData )
{
	if( !recast_findpath_use_caching.GetBool() || !pathResultData.cacheValid )
		return false;
	if( pathResultData.startRef != startRef || pathResultData.endRef != endRef )
		return false;
	return true;
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Builds a waypoint list from the path finding results.
//-----------------------------------------------------------------------------
UnitBaseWaypoint *CRecastMesh::ConstructWaypointsFromStraightPath( pathfind_resultdata_t &findpathData )
{
	UnitBaseWaypoint *pResultPath = NULL;

	for (int i = findpathData.nstraightPath - 1; i >= 0; i--)
	{
		const dtOffMeshConnection *pOffmeshCon = m_navMesh->getOffMeshConnectionByRef( findpathData.straightPathPolys[i] );

		Vector pos( findpathData.straightPath[i*3], findpathData.straightPath[i*3+2], findpathData.straightPath[i*3+1] );

		UnitBaseWaypoint *pNewPath = new UnitBaseWaypoint( pos );
		pNewPath->SetNext( pResultPath );

		// For now, offmesh connections are always considered as edges.
		if( pOffmeshCon )
		{
			if( pResultPath )
				pResultPath->SpecialGoalStatus = CHS_EDGEDOWNDEST;
			pNewPath->SpecialGoalStatus = CHS_EDGEDOWN;
		}
		pResultPath = pNewPath;
	}

	return pResultPath;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
UnitBaseWaypoint * CRecastMesh::FindPath( const Vector &vStart, const Vector &vEnd, float fBeneathLimit, CBaseEntity *pTarget, 
	bool *bIsPartial, const Vector *pStartTestPos )
{
	VPROF_BUDGET( "CRecastMesh::FindPath", "RecastNav" );

	if( !IsLoaded() )
		return NULL;

	UnitBaseWaypoint *pResultPath = NULL;

	dtStatus status;

	dtPolyRef startRef = 0, endRef = 0;

	float spos[3];
	spos[0] = vStart[0];
	spos[1] = vStart[2];
	spos[2] = vStart[1];
	float epos[3];
	epos[0] = vEnd[0];
	epos[1] = vEnd[2];
	epos[2] = vEnd[1];

	bool bHasTargetAndIsObstacle = pTarget && pTarget->GetNavObstacleRef() != NAV_OBSTACLE_INVALID_INDEX;

	status = ComputeAdjustedStartAndEnd( m_navQuery, spos, epos, startRef, endRef, fBeneathLimit, bHasTargetAndIsObstacle, pTarget != NULL, pStartTestPos );
	if( !dtStatusSucceed( status ) )
	{
		return NULL;
	}

	if( recast_findpath_debug.GetBool() )
	{
		NDebugOverlay::Box( Vector(epos[0], epos[2], epos[1] + 16.0f), 
			-Vector(8, 8, 8), Vector(8, 8, 8), 255, 0, 0, 255, 5.0f);
	}

	DoFindPath( m_navQuery, startRef, endRef, spos, epos, bHasTargetAndIsObstacle, m_pathfindData );

	if( m_pathfindData.cacheValid )
	{
		if( bIsPartial ) 
		{
			*bIsPartial = m_pathfindData.isPartial;
		}

		pResultPath = ConstructWaypointsFromStraightPath( m_pathfindData );
	}

	return pResultPath;
}
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Finds the path distance between two points
// Returns: The distance, or -1 if not found.
//-----------------------------------------------------------------------------
float CRecastMesh::FindPathDistance( const Vector &vStart, const Vector &vEnd, CBaseEntity *pTarget, float fBeneathLimit, bool bLimitedSearch )
{
	VPROF_BUDGET( "CRecastMesh::FindPathDistance", "RecastNav" );

	if( !IsLoaded() )
		return -1;

	dtStatus status;

	dtPolyRef startRef = 0, endRef = 0;

	float spos[3];
	spos[0] = vStart[0];
	spos[1] = vStart[2];
	spos[2] = vStart[1];

	float epos[3];
	epos[0] = vEnd[0];
	epos[1] = vEnd[2];
	epos[2] = vEnd[1];

	// Faster search for nearby units. Mainly intended for unit sensing code, so it quickly filters out "unreachable" enemies
	dtNavMeshQuery *navQuery = bLimitedSearch ? m_navQueryLimitedNodes : m_navQuery;

	bool bHasTargetAndIsObstacle = pTarget && pTarget->GetNavObstacleRef() != NAV_OBSTACLE_INVALID_INDEX;

	status = ComputeAdjustedStartAndEnd( navQuery, spos, epos, startRef, endRef, fBeneathLimit, bHasTargetAndIsObstacle, pTarget != NULL, NULL );
	if( !dtStatusSucceed( status ) )
	{
		return NULL;
	}

	if( recast_findpath_debug.GetBool() )
	{
		NDebugOverlay::Box( Vector(epos[0], epos[2], epos[1] + 16.0f), 
			-Vector(8, 8, 8), Vector(8, 8, 8), 255, 0, 0, 255, 5.0f);
	}

	status = DoFindPath( navQuery, startRef, endRef, spos, epos, bHasTargetAndIsObstacle, m_pathfindData );
	if( dtStatusSucceed( status ) && !m_pathfindData.isPartial )
	{
		float fPathDistance = 0;
		Vector vPrev = vEnd;
		for (int i = m_pathfindData.nstraightPath - 1; i >= 0; i--)
		{
			Vector pos( m_pathfindData.straightPath[i*3], m_pathfindData.straightPath[i*3+2], m_pathfindData.straightPath[i*3+1] );
			fPathDistance += pos.DistTo(vPrev);
			vPrev = pos;
		}
		return fPathDistance;
	}
	
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Tests if route between start and end is covered by the navigation
// mesh.
//-----------------------------------------------------------------------------
bool CRecastMesh::TestRoute( const Vector &vStart, const Vector &vEnd )
{
	if( !IsLoaded() )
		return false;

	dtStatus status;

	dtPolyRef startRef;
	dtPolyRef endRef;

	float spos[3];
	spos[0] = vStart[0];
	spos[1] = vStart[2];
	spos[2] = vStart[1];

	float epos[3];
	epos[0] = vEnd[0];
	epos[1] = vEnd[2];
	epos[2] = vEnd[1];

	float polyPickExt[3];
	polyPickExt[0] = 128.0f;
	polyPickExt[1] = 600.0f;
	polyPickExt[2] = 128.0f;

	status = m_navQuery->findNearestPoly(spos, polyPickExt, &m_defaultFilter, &startRef, 0);
	if( !dtStatusSucceed( status ) )
	{
		return false;
	}
	status = m_navQuery->findNearestPoly(epos, polyPickExt, &m_defaultFilter, &endRef, 0);
	if( !dtStatusSucceed( status ) )
	{
		return false;
	}

	dtRaycastHit rayHit;
	rayHit.maxPath = 0;
	rayHit.pathCost = rayHit.t = 0;
	status = m_navQuery->raycast( startRef, spos, epos, &m_defaultFilter, 0, &rayHit );
	if( !dtStatusSucceed( status ) )
	{
		return false;
	}

	return rayHit.t == FLT_MAX;
}

//-----------------------------------------------------------------------------
// Purpose: Adds a temporary obstacle to the navigation mesh (radius version)
//-----------------------------------------------------------------------------
dtObstacleRef CRecastMesh::AddTempObstacle( const Vector &vPos, float radius, float height, unsigned char areaId )
{
	if( !IsLoaded() )
		return 0;
	float pos[3] = {vPos.x, vPos.z, vPos.y};

	dtObstacleRef result;
	dtStatus status = m_tileCache->addObstacle( pos, radius, height, areaId, &result );
	if( !dtStatusSucceed( status ) )
	{
		if( status & DT_BUFFER_TOO_SMALL )
		{
			Warning("CRecastMesh::AddTempObstacle: request buffer too small\n");
		}
		if( status & DT_OUT_OF_MEMORY )
		{
			Warning("CRecastMesh::AddTempObstacle: out of memory\n");
		}
		return 0;
	}
	return result;
}

//-----------------------------------------------------------------------------
// Purpose: Adds a temporary obstacle to the navigation mesh (convex hull version)
//-----------------------------------------------------------------------------
dtObstacleRef CRecastMesh::AddTempObstacle( const Vector &vPos, const Vector *convexHull, const int numConvexHull, float height, unsigned char areaId )
{
	if( !IsLoaded() )
		return 0;

	dtStatus status;
	float pos[3] = {vPos.x, vPos.z, vPos.y};

	float *verts = (float *)stackalloc( numConvexHull * 3 * sizeof( float ) );
	for( int i = 0; i < numConvexHull; i++ )
	{
		verts[(i*3)+0] = convexHull[i].x;
		verts[(i*3)+1] = convexHull[i].z;
		verts[(i*3)+2] = convexHull[i].y;
	}

	dtObstacleRef result;
	status = m_tileCache->addObstacle( pos, verts, numConvexHull, height, areaId, &result );
	if( !dtStatusSucceed( status ) )
	{
		if( status & DT_BUFFER_TOO_SMALL )
		{
			Warning("CRecastMesh::AddTempObstacle: request buffer too small\n");
		}
		if( status & DT_OUT_OF_MEMORY )
		{
			Warning("CRecastMesh::AddTempObstacle: out of memory\n");
		}
		return 0;
	}
	return result;
}

//-----------------------------------------------------------------------------
// Purpose: Removes a temporary obstacle from the navigation mesh
//-----------------------------------------------------------------------------
bool CRecastMesh::RemoveObstacle( const dtObstacleRef ref )
{
	if( !IsLoaded() )
		return false;

	dtStatus status = m_tileCache->removeObstacle( ref );
	if( !dtStatusSucceed( status ) )
	{
		if( status & DT_BUFFER_TOO_SMALL )
		{
			Warning("CRecastMesh::RemoveObstacle: request buffer too small\n");
		}
		if( status & DT_OUT_OF_MEMORY )
		{
			Warning("CRecastMesh::RemoveObstacle: out of memory\n");
		}
		return false;
	}
	return true;
}