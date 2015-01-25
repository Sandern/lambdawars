//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:	
//
//=============================================================================//

#ifndef RECAST_MESH_H
#define RECAST_MESH_H

#ifdef _WIN32
#pragma once
#endif

#include <recast/Recast.h>
#include "detourtilecache/DetourTileCache.h"

class UnitBaseWaypoint;
class CMapMesh;

class dtNavMesh;
class dtTileCache;
class dtNavMeshQuery;

enum SamplePartitionType
{
	SAMPLE_PARTITION_WATERSHED,
	SAMPLE_PARTITION_MONOTONE,
	SAMPLE_PARTITION_LAYERS,
};

/// These are just sample areas to use consistent values across the samples.
/// The use should specify these base on his needs.
enum SamplePolyAreas
{
	SAMPLE_POLYAREA_GROUND,
	SAMPLE_POLYAREA_WATER,
	SAMPLE_POLYAREA_ROAD,
	SAMPLE_POLYAREA_DOOR,
	SAMPLE_POLYAREA_GRASS,
	SAMPLE_POLYAREA_JUMP,
};

enum SamplePolyFlags
{
	SAMPLE_POLYFLAGS_WALK		= 0x01,		// Ability to walk (ground, grass, road)
	SAMPLE_POLYFLAGS_SWIM		= 0x02,		// Ability to swim (water).
	SAMPLE_POLYFLAGS_DOOR		= 0x04,		// Ability to move through doors.
	SAMPLE_POLYFLAGS_JUMP		= 0x08,		// Ability to jump.
	SAMPLE_POLYFLAGS_DISABLED	= 0x10,		// Disabled polygon
	SAMPLE_POLYFLAGS_ALL		= 0xffff	// All abilities.
};

class CRecastMesh
{
public:
	CRecastMesh();
	~CRecastMesh();

	const char *GetName();
	void Init( const char *name );

	virtual void Update( float dt );

	// Load/build 
	//virtual void LoadTestData();

	virtual bool Load( CUtlBuffer &fileBuffer );
	virtual bool Reset();

#ifndef CLIENT_DLL
	virtual bool Build( CMapMesh *pMapMesh );
	virtual bool Save( CUtlBuffer &fileBuffer );
#endif // CLIENT_DLL

#ifdef CLIENT_DLL
	virtual void DebugRender();
#endif // CLIENT_DLL

#ifndef CLIENT_DLL
	// Path find functions
	virtual UnitBaseWaypoint *FindPath( const Vector &vStart, const Vector &vEnd );
#endif // CLIENT_DLL

	// Obstacle management
	dtObstacleRef AddTempObstacle( const Vector &vPos, float radius, float height );
	dtObstacleRef AddTempObstacle( const Vector &vPos, const Vector *convexHull, const int numConvexHull, float height );
	bool RemoveObstacle( const dtObstacleRef ref );

	// Accessors
	dtNavMesh *GetNavMesh() { return m_navMesh; }
	dtNavMeshQuery *GetNavMeshQuery() { return m_navQuery; }

	float GetAgentRadius() { return m_agentRadius; }

protected:
	bool m_keepInterResults;

	float m_cellSize;
	float m_cellHeight;
	float m_agentHeight;
	float m_agentRadius;
	float m_agentMaxClimb;
	float m_agentMaxSlope;
	float m_regionMinSize;
	float m_regionMergeSize;
	float m_edgeMaxLen;
	float m_edgeMaxError;
	float m_vertsPerPoly;
	float m_detailSampleDist;
	float m_detailSampleMaxError;
	int m_partitionType;

	int m_maxTiles;
	int m_maxPolysPerTile;
	float m_tileSize;

	float m_cacheBuildTimeMs;
	int m_cacheCompressedSize;
	int m_cacheRawSize;
	int m_cacheLayerCount;
	int m_cacheBuildMemUsage;

private:
	CUtlString m_Name;
	
	// Data used during build
	unsigned char* m_triareas;
	rcHeightfield* m_solid;
	rcCompactHeightfield* m_chf;
	rcContourSet* m_cset;
	rcPolyMesh* m_pmesh;
	rcConfig m_cfg;	
	rcPolyMeshDetail* m_dmesh;

	struct dtTileCacheAlloc* m_talloc;
	struct dtTileCacheCompressor* m_tcomp;
	struct dtTileCacheMeshProcess *m_tmproc;

	// Data used for path finding
	dtNavMesh* m_navMesh;
	dtTileCache* m_tileCache;
	dtNavMeshQuery* m_navQuery;
};

inline const char *CRecastMesh::GetName()
{
	return m_Name.Get();
}

#endif // RECAST_MESH_H