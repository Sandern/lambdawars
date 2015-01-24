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
	virtual bool Load();
	virtual void Reset();
	
	// Accessors for Units
	CRecastMesh *GetMesh( int index );
	int FindMeshIndex( const char *name );
	CUtlDict< CRecastMesh *, int > &GetMeshes();

	// Used for debugging purposes on client
	virtual dtNavMesh* GetNavMesh( const char *meshName );
	virtual dtNavMeshQuery* GetNavMeshQuery( const char *meshName );

#ifndef CLIENT_DLL
	// Generation methods
	virtual bool Build();
	virtual bool Save();
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

#endif // RECAST_MGR_H