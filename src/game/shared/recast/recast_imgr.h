//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IRECASTMGR_H
#define IRECASTMGR_H

#ifdef _WIN32
#pragma once
#endif

class dtNavMesh;
class dtNavMeshQuery;

abstract_class IRecastMgr
{
public:
	// Used for debugging purposes on client
	virtual dtNavMesh* GetNavMesh( const char *meshName ) = 0;
	virtual dtNavMeshQuery* GetNavMeshQuery( const char *meshName ) = 0;
};

#endif // IRECASTMGR_H