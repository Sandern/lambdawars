//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// Note: Recasts expects "y" to be up, so y and z must be swapped everywhere. 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "recast/recast_mesh.h"
#include "recast/recast_mapmesh.h"

#ifdef CLIENT_DLL
#include "recast/recast_recastdebugdraw.h"
#include "recast/recast_debugdrawmesh.h"
#endif // CLIENT_DLL

#include "detour/DetourNavMesh.h"
#include "detour/DetourNavMeshBuilder.h"
#include "detour/DetourNavMeshQuery.h"
#include "detour/DetourCommon.h"

#ifndef CLIENT_DLL
#include "unit_navigator.h"
#endif // CLIENT_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar recast_edit( "recast_edit", "1", FCVAR_GAMEDLL | FCVAR_CHEAT, "Set to one to interactively edit the Recast Navigation Mesh. Set to zero to leave edit mode." );

enum SamplePartitionType
{
	SAMPLE_PARTITION_WATERSHED,
	SAMPLE_PARTITION_MONOTONE,
	SAMPLE_PARTITION_LAYERS,
};

static CRecastMesh *s_pRecastMesh = NULL;

class BuildContext : public rcContext
{
public:
	virtual void doLog(const rcLogCategory category, const char* msg, const int len) 
	{
		switch( category )
		{
		case RC_LOG_PROGRESS:
			Msg( "%s\n", msg );
			break;
		case RC_LOG_WARNING:
			Warning( "%s\n", msg );
			break;
		case RC_LOG_ERROR:
			Warning( "%s\n", msg );
			break;
		}
	}
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRecastMesh::CRecastMesh() : 
	m_keepInterResults(true),
	m_triareas(0),
	m_solid(0),
	m_chf(0),
	m_cset(0),
	m_pmesh(0),
	m_dmesh(0),
	m_navMesh(0)
{
	m_navQuery = dtAllocNavMeshQuery();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRecastMesh::~CRecastMesh()
{

}

// Test settings temporary
static ConVar recast_cellsize("recast_cellsize", "10.0", FCVAR_REPLICATED);
static ConVar recast_cellheight("recast_cellheight", "10.0", FCVAR_REPLICATED);
static ConVar recast_maxclimb("recast_maxclimb", "18.0", FCVAR_REPLICATED);
static ConVar recast_maxslope("recast_maxslope", "45.0", FCVAR_REPLICATED);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRecastMesh::ResetCommonSettings()
{
	m_cellSize = recast_cellsize.GetFloat();
	m_cellHeight = recast_cellheight.GetFloat();
	m_agentHeight = 72.0f; // => Soldier/human
	m_agentRadius = 18.5f; // => Soldier/human
	m_agentMaxClimb = recast_maxclimb.GetFloat(); //18.0f; // => default unit step height
	m_agentMaxSlope = recast_maxslope.GetFloat(); // Default slope for units
	m_regionMinSize = 8;
	m_regionMergeSize = 20;
	m_edgeMaxLen = 12000.0f;
	m_edgeMaxError = 1.3f;
	m_vertsPerPoly = 6.0f;
	m_detailSampleDist = 600.0f;
	m_detailSampleMaxError = 100.0f;
	m_partitionType = SAMPLE_PARTITION_WATERSHED;
}

static void AddVertex( float* verts, float x, float y, float z )
{
	verts[0] = x;
	verts[1] = z;
	verts[2] = y;
}
static void AddTriangle( int* tris, int p1, int p2, int p3 )
{
	tris[0] = p1;
	tris[1] = p2;
	tris[2] = p3;
}

static void PrintDebugSpans( const char *pStepName, rcHeightfield& solid )
{
	const int w = solid.width;
	const int h = solid.height;

	int nWalkableAreas = 0;
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			for (rcSpan* s = solid.spans[x + y*w]; s; s = s->next)
			{
				if( s->area == RC_WALKABLE_AREA )
					nWalkableAreas += 1;
			}
		}
	}
	Msg( "%s: %d walkable areas\n", pStepName, nWalkableAreas );
}

#if 0
//-----------------------------------------------------------------------------
// Purpose: Build cube
//-----------------------------------------------------------------------------
void CRecastMesh::LoadTestData()
{
	// Create a mesh for testing
	const int nverts = 8;
	float* verts = new float[nverts*3];

	// Create four points for triangles
	const float meshHeight = 720.0f;
	AddVertex( verts, -1024.0f, -1024.0f, 0.0f ); // 0
	AddVertex( verts + 3, -1024.0f, 1024.0f, 0.0f ); // 1
	AddVertex( verts + 6, 1024.0f, 1024.0f, 0.0f ); // 2
	AddVertex( verts + 9, 1024.0f, -1024.0f, 0.0f ); // 3

	AddVertex( verts + 12, -1024.0f, -1024.0f, meshHeight ); // 4
	AddVertex( verts + 15, -1024.0f, 1024.0f, meshHeight ); // 5
	AddVertex( verts + 18, 1024.0f, 1024.0f, meshHeight ); // 6
	AddVertex( verts + 21, 1024.0f, -1024.0f, meshHeight ); // 7

	const int ntris = 12;
	int* tris = new int[ntris*3];

	// Create two triangles using above points
	// Bottom
	AddTriangle( tris, 0, 1, 2 );
	AddTriangle( tris + 3, 0, 2, 3 );

	// Top
	AddTriangle( tris + 6, 4, 6, 5 );
	AddTriangle( tris + 9, 4, 7, 6 );

	// Side 1
	AddTriangle( tris + 12, 0, 5, 1 );
	AddTriangle( tris + 15, 0, 4, 5 );

	// Side 2
	AddTriangle( tris + 18, 1, 5, 2 );
	AddTriangle( tris + 21, 5, 6, 2 );

	// Side 3
	AddTriangle( tris + 24, 2, 6, 3 );
	AddTriangle( tris + 27, 6, 7, 3 );

	// Side 4
	AddTriangle( tris + 30, 3, 7, 0 );
	AddTriangle( tris + 33, 7, 4, 0 );

	m_verts = verts;
	m_nverts = nverts;
	m_tris = tris;
	m_ntris = ntris;

	// Calculate normals.
	m_normals = new float[ntris*3];
	for (int i = 0; i < ntris*3; i += 3)
	{
		const float* v0 = &m_verts[m_tris[i]*3];
		const float* v1 = &m_verts[m_tris[i+1]*3];
		const float* v2 = &m_verts[m_tris[i+2]*3];
		float e0[3], e1[3];
		for (int j = 0; j < 3; ++j)
		{
			e0[j] = v1[j] - v0[j];
			e1[j] = v2[j] - v0[j];
		}
		float* n = &m_normals[i];
		n[0] = e0[1]*e1[2] - e0[2]*e1[1];
		n[1] = e0[2]*e1[0] - e0[0]*e1[2];
		n[2] = e0[0]*e1[1] - e0[1]*e1[0];
		float d = sqrtf(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
		if (d > 0)
		{
			d = 1.0f/d;
			n[0] *= d;
			n[1] *= d;
			n[2] *= d;
		}
	}
}
#endif // 0

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRecastMesh::Build()
{
	double fStartTime = Plat_FloatTime();

	Reset(); // Clean any existing data
	ResetCommonSettings(); // Reset parameters

	BuildContext ctx;

	ctx.enableLog( true );

	int m_partitionType = SAMPLE_PARTITION_WATERSHED;

	//LoadTestData();
	CMapMesh *pMapMesh = new CMapMesh();
	if( !pMapMesh->Load() )
	{
		Warning("CRecastMesh::Load: failed to load map data!\n");
		return false;
	}

	const int nverts = pMapMesh->GetNumVerts();
	const float *verts = pMapMesh->GetVerts();
	const int ntris = pMapMesh->GetNumTris();
	const int *tris = pMapMesh->GetTris();

	//
	// Step 1. Initialize build config.
	//
	
	// Init build configuration from GUI
	V_memset(&m_cfg, 0, sizeof(m_cfg));
	m_cfg.cs = m_cellSize;
	m_cfg.ch = m_cellHeight;
	m_cfg.walkableSlopeAngle = m_agentMaxSlope;
	m_cfg.walkableHeight = (int)ceilf(m_agentHeight / m_cfg.ch);
	m_cfg.walkableClimb = (int)floorf(m_agentMaxClimb / m_cfg.ch);
	m_cfg.walkableRadius = (int)ceilf(m_agentRadius / m_cfg.cs);
	m_cfg.maxEdgeLen = (int)(m_edgeMaxLen / m_cellSize);
	m_cfg.maxSimplificationError = m_edgeMaxError;
	m_cfg.minRegionArea = (int)rcSqr(m_regionMinSize);		// Note: area = size*size
	m_cfg.mergeRegionArea = (int)rcSqr(m_regionMergeSize);	// Note: area = size*size
	m_cfg.maxVertsPerPoly = (int)m_vertsPerPoly;
	m_cfg.detailSampleDist = m_detailSampleDist < 0.9f ? 0 : m_cellSize * m_detailSampleDist;
	m_cfg.detailSampleMaxError = m_cellHeight * m_detailSampleMaxError;

	// Set the area where the navigation will be build.
	// Here the bounds of the input mesh are used, but the
	// area could be specified by an user defined box, etc.
	/*static Vector worldMin( MIN_COORD_FLOAT, MIN_COORD_FLOAT, MIN_COORD_FLOAT );
	static Vector worldMax( MAX_COORD_FLOAT, MAX_COORD_FLOAT, MAX_COORD_FLOAT );
	rcVcopy(m_cfg.bmin, worldMin.Base());
	rcVcopy(m_cfg.bmax, worldMax.Base());*/
	rcCalcBounds( verts, nverts, m_cfg.bmin, m_cfg.bmax );
	rcCalcGridSize(m_cfg.bmin, m_cfg.bmax, m_cfg.cs, &m_cfg.width, &m_cfg.height);

	Msg("walkableHeight: %d, walkableClimb: %d, walkableRadius: %d, walkableSlopeAngle: %f\n", m_cfg.walkableHeight, m_cfg.walkableClimb, m_cfg.walkableRadius, m_cfg.walkableSlopeAngle);
	Msg("Grid size: bmin: %f %f %f bmax: %f %f %f, width: %d, height: %d\n", m_cfg.bmin[0], m_cfg.bmin[1], m_cfg.bmin[2],
		m_cfg.bmax[0], m_cfg.bmax[1], m_cfg.bmax[2], m_cfg.width, m_cfg.height);

	//
	// Step 2. Rasterize input polygon soup.
	//
	
	// Allocate voxel heightfield where we rasterize our input data to.
	m_solid = rcAllocHeightfield();
	if (!m_solid)
	{
		ctx.log(RC_LOG_ERROR, "buildNavigation: Out of memory 'solid'.");
		return false;
	}
	if (!rcCreateHeightfield(&ctx, *m_solid, m_cfg.width, m_cfg.height, m_cfg.bmin, m_cfg.bmax, m_cfg.cs, m_cfg.ch))
	{
		ctx.log(RC_LOG_ERROR, "buildNavigation: Could not create solid heightfield.");
		return false;
	}
	
	// Allocate array that can hold triangle area types.
	// If you have multiple meshes you need to process, allocate
	// and array which can hold the max number of triangles you need to process.
	m_triareas = new unsigned char[ntris];
	if (!m_triareas)
	{
		ctx.log(RC_LOG_ERROR, "buildNavigation: Out of memory 'm_triareas' (%d).", ntris);
		return false;
	}
	
	// Find triangles which are walkable based on their slope and rasterize them.
	// If your input data is multiple meshes, you can transform them here, calculate
	// the are type for each of the meshes and rasterize them.
	V_memset(m_triareas, 0, ntris*sizeof(unsigned char));
	rcMarkWalkableTriangles(&ctx, m_cfg.walkableSlopeAngle, verts, nverts, tris, ntris, m_triareas);

	int nWalkableAreas = 0;
	for( int i = 0; i < ntris; i++ )
	{
		//Msg("Triangle %d is walkable? %d\n", i, m_triareas[i]);
		if( m_triareas[i] > 0 )
			nWalkableAreas += 1;
	}
	//Msg( "RecastDebug: %d walkable triangles\n", nWalkableAreas );

	rcRasterizeTriangles(&ctx, verts, nverts, tris, m_triareas, ntris, *m_solid, m_cfg.walkableClimb);
	//PrintDebugSpans( "After rcRasterizeTriangles", *m_solid );

#if 0
	const float walkableThr = cosf(m_cfg.walkableSlopeAngle/180.0f*RC_PI);

	float norm[3];
	
	Msg("walkableThr: %f\n", walkableThr);
	for (int i = 0; i < ntris; ++i)
	{
		const int* tri = &tris[i*3];
		calcTriNormal(&verts[tri[0]*3], &verts[tri[1]*3], &verts[tri[2]*3], norm);

		// Check if the face is walkable.
		Msg("Triangle %d norm: %f %f %f\n", i, norm[0], norm[1], norm[2]);
		//if (norm[1] > walkableThr)
		//	areas[i] = RC_WALKABLE_AREA;
	}
#endif // 0

	if (!m_keepInterResults)
	{
		delete [] m_triareas;
		m_triareas = 0;
	}
	
	//
	// Step 3. Filter walkables surfaces.
	//
	
	// Once all geoemtry is rasterized, we do initial pass of filtering to
	// remove unwanted overhangs caused by the conservative rasterization
	// as well as filter spans where the character cannot possibly stand.
	rcFilterLowHangingWalkableObstacles(&ctx, m_cfg.walkableClimb, *m_solid);
	//PrintDebugSpans( "After rcFilterLowHangingWalkableObstacles", *m_solid );
	rcFilterLedgeSpans(&ctx, m_cfg.walkableHeight, m_cfg.walkableClimb, *m_solid);
	//PrintDebugSpans( "After rcFilterLedgeSpans", *m_solid );
	rcFilterWalkableLowHeightSpans(&ctx, m_cfg.walkableHeight, *m_solid);
	//PrintDebugSpans( "After rcFilterWalkableLowHeightSpans", *m_solid );


	//
	// Step 4. Partition walkable surface to simple regions.
	//

	// Compact the heightfield so that it is faster to handle from now on.
	// This will result more cache coherent data as well as the neighbours
	// between walkable cells will be calculated.
	m_chf = rcAllocCompactHeightfield();
	if (!m_chf)
	{
		ctx.log(RC_LOG_ERROR, "buildNavigation: Out of memory 'chf'.");
		return false;
	}
	if (!rcBuildCompactHeightfield(&ctx, m_cfg.walkableHeight, m_cfg.walkableClimb, *m_solid, *m_chf))
	{
		ctx.log(RC_LOG_ERROR, "buildNavigation: Could not build compact data");
		return false;
	}
	
	if (!m_keepInterResults)
	{
		rcFreeHeightField(m_solid);
		m_solid = 0;
	}
		
	// Erode the walkable area by agent radius.
	if (!rcErodeWalkableArea(&ctx, m_cfg.walkableRadius, *m_chf))
	{
		ctx.log(RC_LOG_ERROR, "buildNavigation: Could not erode.");
		return false;
	}

	// (Optional) Mark areas.
	/*const ConvexVolume* vols = m_geom->getConvexVolumes();
	for (int i  = 0; i < m_geom->getConvexVolumeCount(); ++i)
		rcMarkConvexPolyArea(m_ctx, vols[i].verts, vols[i].nverts, vols[i].hmin, vols[i].hmax, (unsigned char)vols[i].area, *m_chf);*/

	
	// Partition the heightfield so that we can use simple algorithm later to triangulate the walkable areas.
	// There are 3 martitioning methods, each with some pros and cons:
	// 1) Watershed partitioning
	//   - the classic Recast partitioning
	//   - creates the nicest tessellation
	//   - usually slowest
	//   - partitions the heightfield into nice regions without holes or overlaps
	//   - the are some corner cases where this method creates produces holes and overlaps
	//      - holes may appear when a small obstacles is close to large open area (triangulation can handle this)
	//      - overlaps may occur if you have narrow spiral corridors (i.e stairs), this make triangulation to fail
	//   * generally the best choice if you precompute the nacmesh, use this if you have large open areas
	// 2) Monotone partioning
	//   - fastest
	//   - partitions the heightfield into regions without holes and overlaps (guaranteed)
	//   - creates long thin polygons, which sometimes causes paths with detours
	//   * use this if you want fast navmesh generation
	// 3) Layer partitoining
	//   - quite fast
	//   - partitions the heighfield into non-overlapping regions
	//   - relies on the triangulation code to cope with holes (thus slower than monotone partitioning)
	//   - produces better triangles than monotone partitioning
	//   - does not have the corner cases of watershed partitioning
	//   - can be slow and create a bit ugly tessellation (still better than monotone)
	//     if you have large open areas with small obstacles (not a problem if you use tiles)
	//   * good choice to use for tiled navmesh with medium and small sized tiles
	
	if (m_partitionType == SAMPLE_PARTITION_WATERSHED)
	{
		// Prepare for region partitioning, by calculating distance field along the walkable surface.
		if (!rcBuildDistanceField(&ctx, *m_chf))
		{
			ctx.log(RC_LOG_ERROR, "buildNavigation: Could not build distance field.");
			return false;
		}
		
		// Partition the walkable surface into simple regions without holes.
		if (!rcBuildRegions(&ctx, *m_chf, 0, m_cfg.minRegionArea, m_cfg.mergeRegionArea))
		{
			ctx.log(RC_LOG_ERROR, "buildNavigation: Could not build watershed regions.");
			return false;
		}
	}
	else if (m_partitionType == SAMPLE_PARTITION_MONOTONE)
	{
		// Partition the walkable surface into simple regions without holes.
		// Monotone partitioning does not need distancefield.
		if (!rcBuildRegionsMonotone(&ctx, *m_chf, 0, m_cfg.minRegionArea, m_cfg.mergeRegionArea))
		{
			ctx.log(RC_LOG_ERROR, "buildNavigation: Could not build monotone regions.");
			return false;
		}
	}
	else // SAMPLE_PARTITION_LAYERS
	{
		// Partition the walkable surface into simple regions without holes.
		if (!rcBuildLayerRegions(&ctx, *m_chf, 0, m_cfg.minRegionArea))
		{
			ctx.log(RC_LOG_ERROR, "buildNavigation: Could not build layer regions.");
			return false;
		}
	}
	
	//
	// Step 5. Trace and simplify region contours.
	//
	
	// Create contours.
	m_cset = rcAllocContourSet();
	if (!m_cset)
	{
		ctx.log(RC_LOG_ERROR, "buildNavigation: Out of memory 'cset'.");
		return false;
	}
	if (!rcBuildContours(&ctx, *m_chf, m_cfg.maxSimplificationError, m_cfg.maxEdgeLen, *m_cset))
	{
		Warning("buildNavigation: Could not create contours.\n");
		return false;
	}

	Msg("After rcBuildContours: %d contours\n", m_cset->nconts);
	
	//
	// Step 6. Build polygons mesh from contours.
	//
	
	// Build polygon navmesh from the contours.
	m_pmesh = rcAllocPolyMesh();
	if (!m_pmesh)
	{
		ctx.log(RC_LOG_ERROR, "buildNavigation: Out of memory 'pmesh'.");
		return false;
	}
	if (!rcBuildPolyMesh(&ctx, *m_cset, m_cfg.maxVertsPerPoly, *m_pmesh))
	{
		ctx.log(RC_LOG_ERROR, "buildNavigation: Could not triangulate contours.");
		return false;
	}
	
	//
	// Step 7. Create detail mesh which allows to access approximate height on each polygon.
	//
	
	m_dmesh = rcAllocPolyMeshDetail();
	if (!m_dmesh)
	{
		ctx.log(RC_LOG_ERROR, "buildNavigation: Out of memory 'pmdtl'.");
		return false;
	}

	if (!rcBuildPolyMeshDetail(&ctx, *m_pmesh, *m_chf, m_cfg.detailSampleDist, m_cfg.detailSampleMaxError, *m_dmesh))
	{
		ctx.log(RC_LOG_ERROR, "buildNavigation: Could not build detail mesh.");
		return false;
	}

	if (!m_keepInterResults)
	{
		rcFreeCompactHeightfield(m_chf);
		m_chf = 0;
		rcFreeContourSet(m_cset);
		m_cset = 0;
	}

	// At this point the navigation mesh data is ready, you can access it from m_pmesh.
	// See duDebugDrawPolyMesh or dtCreateNavMeshData as examples how to access the data.

	// At this point the navigation mesh data is ready, you can access it from m_pmesh.
	// See duDebugDrawPolyMesh or dtCreateNavMeshData as examples how to access the data.
	
	//
	// (Optional) Step 8. Create Detour data from Recast poly mesh.
	//
	
	// The GUI may allow more max points per polygon than Detour can handle.
	// Only build the detour navmesh if we do not exceed the limit.
	if (m_cfg.maxVertsPerPoly <= DT_VERTS_PER_POLYGON)
	{
		unsigned char* navData = 0;
		int navDataSize = 0;

		// Update poly flags from areas.
		for (int i = 0; i < m_pmesh->npolys; ++i)
		{
			if (m_pmesh->areas[i] == RC_WALKABLE_AREA)
				m_pmesh->areas[i] = SAMPLE_POLYAREA_GROUND;
				
			if (m_pmesh->areas[i] == SAMPLE_POLYAREA_GROUND ||
				m_pmesh->areas[i] == SAMPLE_POLYAREA_GRASS ||
				m_pmesh->areas[i] == SAMPLE_POLYAREA_ROAD)
			{
				m_pmesh->flags[i] = SAMPLE_POLYFLAGS_WALK;
			}
			else if (m_pmesh->areas[i] == SAMPLE_POLYAREA_WATER)
			{
				m_pmesh->flags[i] = SAMPLE_POLYFLAGS_SWIM;
			}
			else if (m_pmesh->areas[i] == SAMPLE_POLYAREA_DOOR)
			{
				m_pmesh->flags[i] = SAMPLE_POLYFLAGS_WALK | SAMPLE_POLYFLAGS_DOOR;
			}
		}


		dtNavMeshCreateParams params;
		memset(&params, 0, sizeof(params));
		params.verts = m_pmesh->verts;
		params.vertCount = m_pmesh->nverts;
		params.polys = m_pmesh->polys;
		params.polyAreas = m_pmesh->areas;
		params.polyFlags = m_pmesh->flags;
		params.polyCount = m_pmesh->npolys;
		params.nvp = m_pmesh->nvp;
		params.detailMeshes = m_dmesh->meshes;
		params.detailVerts = m_dmesh->verts;
		params.detailVertsCount = m_dmesh->nverts;
		params.detailTris = m_dmesh->tris;
		params.detailTriCount = m_dmesh->ntris;
#if 0
		params.offMeshConVerts = m_geom->getOffMeshConnectionVerts();
		params.offMeshConRad = m_geom->getOffMeshConnectionRads();
		params.offMeshConDir = m_geom->getOffMeshConnectionDirs();
		params.offMeshConAreas = m_geom->getOffMeshConnectionAreas();
		params.offMeshConFlags = m_geom->getOffMeshConnectionFlags();
		params.offMeshConUserID = m_geom->getOffMeshConnectionId();
		params.offMeshConCount = m_geom->getOffMeshConnectionCount();
#else
		// Not sure what this is..
#endif // 0
		params.walkableHeight = m_agentHeight;
		params.walkableRadius = m_agentRadius;
		params.walkableClimb = m_agentMaxClimb;
		rcVcopy(params.bmin, m_pmesh->bmin);
		rcVcopy(params.bmax, m_pmesh->bmax);
		params.cs = m_cfg.cs;
		params.ch = m_cfg.ch;
		params.buildBvTree = true;
		
		if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
		{
			ctx.log(RC_LOG_ERROR, "Could not build Detour navmesh.");
			return false;
		}
		
		m_navMesh = dtAllocNavMesh();
		if (!m_navMesh)
		{
			dtFree(navData);
			ctx.log(RC_LOG_ERROR, "Could not create Detour navmesh");
			return false;
		}
		
		dtStatus status;
		
		status = m_navMesh->init(navData, navDataSize, DT_TILE_FREE_DATA);
		if (dtStatusFailed(status))
		{
			dtFree(navData);
			ctx.log(RC_LOG_ERROR, "Could not init Detour navmesh");
			return false;
		}
		
		status = m_navQuery->init(m_navMesh, 2048);
		if (dtStatusFailed(status))
		{
			ctx.log(RC_LOG_ERROR, "Could not init Detour navmesh query");
			return false;
		}
	}
	
	ctx.stopTimer(RC_TIMER_TOTAL);

	// Show performance stats.
	//duLogBuildTimes(ctx, ctx.getAccumulatedTime(RC_TIMER_TOTAL));
	ctx.log(RC_LOG_PROGRESS, ">> Polymesh: %d vertices  %d polygons", m_pmesh->nverts, m_pmesh->npolys);
	
	//m_totalBuildTimeMs = m_ctx->getAccumulatedTime(RC_TIMER_TOTAL)/1000.0f;


	DevMsg( "CRecastMesh: Generated navigation mesh in %f seconds\n", Plat_FloatTime() - fStartTime );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRecastMesh::Load()
{
	return Build();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRecastMesh::Reset()
{
	// Cleanup Nav mesh data
	if( m_triareas )
	{
		delete [] m_triareas;
		m_triareas = 0;
	}

	if( m_solid )
	{
		rcFreeHeightField(m_solid);
		m_solid = 0;
	}

	if( m_chf )
	{
		rcFreeCompactHeightfield(m_chf);
		m_chf = 0;
	}

	if( m_cset )
	{
		rcFreeContourSet(m_cset);
		m_cset = 0;
	}

	if( m_pmesh )
	{
		rcFreePolyMesh(m_pmesh);
		m_pmesh = 0;
	}

	if( m_dmesh )
	{
		rcFreePolyMeshDetail(m_dmesh);
		m_dmesh = 0;
	}

	if( m_navMesh )
	{
		dtFreeNavMesh(m_navMesh);
		m_navMesh = 0;
	}

	return true;
}

#ifdef CLIENT_DLL
ConVar recast_draw_trimeshslope("recast_draw_trimeshslope", "0");
ConVar recast_draw_contours("recast_draw_contours", "0");
ConVar recast_draw_polymesh("recast_draw_polymesh", "0");
ConVar recast_draw_polymeshdetail("recast_draw_polymeshdetail", "0");

//-----------------------------------------------------------------------------
// Purpose: Draws the mesh
//-----------------------------------------------------------------------------
void CRecastMesh::DebugRender()
{
	//if( !recast_edit.GetBool() )
	//	return;

	DebugDrawMesh dd;

#if defined(_DEBUG) || defined(CALC_GEOM_NORMALS)
	if( recast_draw_trimeshslope.GetBool() && m_normals )
	{
		const float texScale = 1.0f / (m_cellSize * 10.0f);

		duDebugDrawTriMeshSlope(&dd, m_verts, m_nverts,
								m_tris, m_normals, m_ntris,
								m_agentMaxSlope, texScale);
	}
#endif // CALC_GEOM_NORMALS

	if( recast_draw_contours.GetBool() && m_cset != NULL )
	{
		duDebugDrawContours(&dd, *m_cset);
	}

	if( recast_draw_polymesh.GetBool() )
	{
		duDebugDrawPolyMesh(&dd, *m_pmesh);
	}

	if( recast_draw_polymeshdetail.GetBool() )
	{
		duDebugDrawPolyMeshDetail(&dd, *m_dmesh);
	}

	//duDebugDrawNavMeshPolysWithFlags(&dd, *m_navMesh, SAMPLE_POLYFLAGS_DISABLED, duRGBA(0,0,0,128));
}
#endif // CLIENT_DLL

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
UnitBaseWaypoint * CRecastMesh::FindPath( const Vector &vStart, const Vector &vEnd )
{
	UnitBaseWaypoint *pResultPath = NULL;

	dtQueryFilter m_filter;
	m_filter.setIncludeFlags(SAMPLE_POLYFLAGS_ALL ^ SAMPLE_POLYFLAGS_DISABLED);
	m_filter.setExcludeFlags(0);

#define MAX_POLYS 256
	dtPolyRef m_polys[MAX_POLYS];
	int m_npolys;

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
	polyPickExt[0] = 2;
	polyPickExt[1] = 4;
	polyPickExt[2] = 2;

	int m_straightPathOptions = 0;
	float m_straightPath[MAX_POLYS*3];
	unsigned char m_straightPathFlags[MAX_POLYS];
	dtPolyRef m_straightPathPolys[MAX_POLYS];
	int m_nstraightPath;

	m_navQuery->findNearestPoly(spos, polyPickExt, &m_filter, &startRef, 0);
	m_navQuery->findNearestPoly(epos, polyPickExt, &m_filter, &endRef, 0);

	m_navQuery->findPath(startRef, endRef, spos, epos, &m_filter, m_polys, &m_npolys, MAX_POLYS);
	m_nstraightPath = 0;
	if (m_npolys)
	{
		// In case of partial path, make sure the end point is clamped to the last polygon.
		float epos2[3];
		dtVcopy(epos2, epos);
		if (m_polys[m_npolys-1] != endRef)
			m_navQuery->closestPointOnPoly(m_polys[m_npolys-1], epos, epos2, 0);
				
		m_navQuery->findStraightPath(spos, epos2, m_polys, m_npolys,
										m_straightPath, m_straightPathFlags,
										m_straightPathPolys, &m_nstraightPath, MAX_POLYS, m_straightPathOptions);

		pResultPath = new UnitBaseWaypoint( vEnd );
		for (int i = m_nstraightPath - 1; i >= 0; i--)
		{
			Vector pos( m_straightPath[i*3], m_straightPath[i*3+2], m_straightPath[i*3+1] );

			UnitBaseWaypoint *pNewPath = new UnitBaseWaypoint( pos );
			pNewPath->SetNext( pResultPath );
			pResultPath = pNewPath;
		}
	}

	return pResultPath;
}
#endif // CLIENT_DLL
