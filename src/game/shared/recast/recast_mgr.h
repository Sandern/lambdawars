//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:	
//
//=============================================================================//

#ifndef RECAST_MGR_H
#define RECAST_MGR_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/utldict.h"
#include "tier1/utlmap.h"
#include "detourtilecache/DetourTileCache.h"
#include "recast_imgr.h"

class CRecastMesh;
class CMapMesh;

typedef struct NavObstacle_t
{
	int meshIndex;
	dtObstacleRef ref;
} NavObstacleRef_t;

typedef struct NavObstacleArray_t
{
	NavObstacleArray_t() {}
	NavObstacleArray_t( const NavObstacleArray_t &ref )
	{
		obs =  ref.obs;
	}

	CUtlVector< NavObstacle_t > obs;
} NavObstacleArray_t;

class CRecastMgr : public IRecastMgr
{
public:
	CRecastMgr();
	~CRecastMgr();

	virtual void Init();

	virtual void Update( float dt );

	// Load methods
	virtual bool InitDefaultMeshes();
	virtual bool Load();
	virtual void Reset();
	
	// Accessors for Units
	bool HasMeshes();
	CRecastMesh *GetMesh( int index );
	CRecastMesh *GetMesh( const char *name );
	int FindMeshIndex( const char *name );
	CUtlDict< CRecastMesh *, int > &GetMeshes();

	int FindBestMeshForRadiusHeight( float radius, float height );
	int FindBestMeshForEntity( CBaseEntity *pEntity );

	// Used for debugging purposes on client
	virtual dtNavMesh* GetNavMesh( const char *meshName );
	virtual dtNavMeshQuery* GetNavMeshQuery( const char *meshName );
	virtual IMapMesh* GetMapMesh();
	
#ifndef CLIENT_DLL
	// Generation methods
	virtual bool LoadMapMesh( bool bLog = true, bool bDynamicOnly = false, 
		const Vector &vMinBounds = vec3_origin, const Vector &vMaxBounds = vec3_origin );
	virtual bool Build();
	virtual bool Save();

	virtual bool IsMeshBuildDisabled( const char *meshName );

	// Rebuilds mesh partial. Clears and rebuilds tiles touching the bounds.
	virtual bool RebuildPartial( const Vector &vMins, const Vector& vMaxs );
	virtual void UpdateRebuildPartial();

	// threaded mesh building
	static void ThreadedBuildMesh( CRecastMesh *&pMesh );
	static void ThreadedRebuildPartialMesh( CRecastMesh *&pMesh );
#endif // CLIENT_DLL

	// Obstacle management
	virtual bool AddEntRadiusObstacle( CBaseEntity *pEntity, float radius, float height );
	virtual bool AddEntBoxObstacle( CBaseEntity *pEntity, const Vector &mins, const Vector &maxs, float height );
	virtual bool RemoveEntObstacles( CBaseEntity *pEntity );

	// Debug
#ifdef CLIENT_DLL
	void DebugRender();
#endif // CLIENT_DLL

protected:
#ifndef CLIENT_DLL
	const char *GetFilename( void ) const;
	virtual bool BuildMesh( CMapMesh *m_pMapMesh, const char *name );
#endif // CLIENT_DLL

	NavObstacleArray_t &FindOrCreateObstacle( CBaseEntity *pEntity );

private:
	bool m_bLoaded;

#ifndef CLIENT_DLL
	// Map mesh used for generation. May not be set.
	CMapMesh *m_pMapMesh;
	// Pending partial updates
	struct PartialMeshUpdate_t
	{
		Vector vMins;
		Vector vMaxs;
	};
	CUtlVector< PartialMeshUpdate_t > m_pendingPartialMeshUpdates;
#endif // CLIENT_DLL

	CUtlDict< CRecastMesh *, int > m_Meshes;

	CUtlMap< EHANDLE, NavObstacleArray_t > m_Obstacles;
};

CRecastMgr &RecastMgr();

inline CUtlDict< CRecastMesh *, int > &CRecastMgr::GetMeshes()
{
	return m_Meshes;
}

inline bool CRecastMgr::HasMeshes()
{
	return m_bLoaded;
}

inline CRecastMesh *CRecastMgr::GetMesh( const char *name )
{
	int idx = FindMeshIndex( name );
	if( idx != -1 )
		return GetMesh( idx );
	return NULL;
}

#endif // RECAST_MGR_H