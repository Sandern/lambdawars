//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "recast/recast_mesh.h"

#ifdef CLIENT_DLL
#include "recast/recast_recastdebugdraw.h"
#include "recast/recast_debugdrawmesh.h"
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
CRecastMesh::CRecastMesh() : m_keepInterResults(true)
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRecastMesh::~CRecastMesh()
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRecastMesh::resetCommonSettings()
{
	const float scale = 100.0f;

	m_cellSize = 32.0f; //0.3f * scale;
	m_cellHeight = 0.2f * scale;
	m_agentHeight = 2.0f * scale;
	m_agentRadius = 0.6f * scale;
	m_agentMaxClimb = 0.9f * scale;
	m_agentMaxSlope = 45.0f;
	m_regionMinSize = 8 * scale;
	m_regionMergeSize = 20 * scale;
	m_edgeMaxLen = 12.0f * scale;
	m_edgeMaxError = 1.3f * scale;
	m_vertsPerPoly = 6.0f;
	m_detailSampleDist = 6.0f * scale;
	m_detailSampleMaxError = 1.0f;
	m_partitionType = SAMPLE_PARTITION_WATERSHED;
}

static void AddVertex( float* verts, float x, float y, float z )
{
	verts[0] = x;
	verts[1] = y;
	verts[2] = z;
}
static void AddTriangle( int* tris, int p1, int p2, int p3 )
{
	tris[0] = p1;
	tris[1] = p2;
	tris[2] = p3;
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
bool CRecastMesh::Load()
{
	resetCommonSettings();

	BuildContext ctx;

	ctx.enableLog( true );

	int m_partitionType = SAMPLE_PARTITION_WATERSHED;

	// Create a mesh for testing
	float* verts = new float[8*3];
	const int nverts = 8;

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

	int* tris = new int[4*3];
	const int ntris = 4;

	// Create two triangles using above points
	// Bottom
	AddTriangle( tris, 0, 2, 1 );
	AddTriangle( tris + 3, 0, 3, 2 );

	// Top
	AddTriangle( tris + 6, 4, 5, 6 );
	AddTriangle( tris + 9, 4, 6, 7 );

	// Side 1
	//AddTriangle( tris + 9, 0, 4, 5 );
	//AddTriangle( tris + 9, 0, 4, 5 );

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

	//
	// Step 1. Initialize build config.
	//
	
	// Init build configuration from GUI
	V_memset(&m_cfg, 0, sizeof(m_cfg));
	/*m_cfg.cs = 25.0f;
	m_cfg.ch = 25.0f;
	m_cfg.walkableSlopeAngle = 0.7f; 
	m_cfg.walkableHeight = (int)ceilf(96.0f / m_cfg.ch);
	m_cfg.walkableClimb = (int)floorf(512.0f / m_cfg.ch);
	m_cfg.walkableRadius = (int)ceilf(32.0f / m_cfg.cs);
	m_cfg.maxEdgeLen = (int)(25.0f / 25.0f);
	m_cfg.maxSimplificationError = 1.0f;
	m_cfg.minRegionArea = (int)rcSqr(25.0f*25.0f);		// Note: area = size*size
	m_cfg.mergeRegionArea = (int)rcSqr(100.0f*100.0f);	// Note: area = size*size
	m_cfg.maxVertsPerPoly = (int)6;
	m_cfg.detailSampleDist = 6 < 0.9f ? 0 : 25.0f * 6;
	m_cfg.detailSampleMaxError = 100 * 1;*/

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
	static Vector worldMin( MIN_COORD_FRACTION, MIN_COORD_FRACTION, MIN_COORD_FRACTION );
	static Vector worldMax( MAX_COORD_FRACTION, MAX_COORD_FRACTION, MAX_COORD_FRACTION );
	rcVcopy(m_cfg.bmin, worldMin.Base());
	rcVcopy(m_cfg.bmax, worldMax.Base());
	rcCalcGridSize(m_cfg.bmin, m_cfg.bmax, m_cfg.cs, &m_cfg.width, &m_cfg.height);

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
	rcRasterizeTriangles(&ctx, verts, nverts, tris, m_triareas, ntris, *m_solid, m_cfg.walkableClimb);

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

	for( int i = 0; i < ntris; i++ )
	{
		Msg("Triangle %d is walkable? %d\n", i, m_triareas[i]);
	}

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
	rcFilterLedgeSpans(&ctx, m_cfg.walkableHeight, m_cfg.walkableClimb, *m_solid);
	rcFilterWalkableLowHeightSpans(&ctx, m_cfg.walkableHeight, *m_solid);


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

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRecastMesh::Reset()
{
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

	const float texScale = 1.0f / (m_cellSize * 10.0f);

	if( recast_draw_trimeshslope.GetBool() )
	{
		duDebugDrawTriMeshSlope(&dd, m_verts, m_nverts,
								m_tris, m_normals, m_ntris,
								0.7, texScale);
	}

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

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRecastMesh *GetRecastNavMesh()
{
	return s_pRecastMesh;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void InitRecastMesh()
{
	s_pRecastMesh = new CRecastMesh();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DestroyRecastMesh()
{
	if( !s_pRecastMesh )
	{
		return;
	}

	delete s_pRecastMesh;
	s_pRecastMesh = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void LoadRecastMesh()
{
	s_pRecastMesh->Load();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ResetRecastMesh()
{
	s_pRecastMesh->Reset();
}
