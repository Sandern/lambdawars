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
	virtual bool LoadMapMesh();
	virtual bool Build();
	virtual bool Save();

	// threaded mesh building
	static void ThreadedBuildMesh( CRecastMesh *&pMesh );
#endif // CLIENT_DLL

	// Obstacle management
	virtual void AddEntRadiusObstacle( CBaseEntity *pEntity, float radius, float height );
	virtual void AddEntBoxObstacle( CBaseEntity *pEntity, const Vector &mins, const Vector &maxs, float height );
	virtual void RemoveEntObstacles( CBaseEntity *pEntity );

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
#ifndef CLIENT_DLL
	CMapMesh *m_pMapMesh;
#endif // CLIENT_DLL

	CUtlDict< CRecastMesh *, int > m_Meshes;

	CUtlMap< int, NavObstacleArray_t > m_Obstacles;
};

CRecastMgr &RecastMgr();

inline CUtlDict< CRecastMesh *, int > &CRecastMgr::GetMeshes()
{
	return m_Meshes;
}

inline bool CRecastMgr::HasMeshes()
{
	return m_Meshes.Count() != 0;
}

inline CRecastMesh *CRecastMgr::GetMesh( const char *name )
{
	int idx = FindMeshIndex( name );
	if( idx != -1 )
		return GetMesh( idx );
	return NULL;
}

#endif // RECAST_MGR_H