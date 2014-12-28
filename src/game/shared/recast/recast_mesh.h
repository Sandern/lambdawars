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

class CRecastMesh
{
public:
	CRecastMesh();
	~CRecastMesh();

	virtual bool Load();
	virtual bool Reset();

	virtual void DebugRender();

private:
	rcHeightfield* m_solid;
	rcCompactHeightfield* m_chf;
	rcContourSet* m_cset;
	rcPolyMesh* m_pmesh;
	rcConfig m_cfg;	
	rcPolyMeshDetail* m_dmesh;
};

void InitRecastMesh();
void DestroyRecastMesh();

void LoadRecastMesh();
void ResetRecastMesh();

#endif // RECAST_MESH_H