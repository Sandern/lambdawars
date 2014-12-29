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


	virtual void LoadTestData();
	virtual void ResetCommonSettings();

	virtual bool Load();
	virtual bool Reset();

#ifdef CLIENT_DLL
	virtual void DebugRender();
#endif // CLIENT_DLL

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

private:
	unsigned char* m_triareas;
	rcHeightfield* m_solid;
	rcCompactHeightfield* m_chf;
	rcContourSet* m_cset;
	rcPolyMesh* m_pmesh;
	rcConfig m_cfg;	
	rcPolyMeshDetail* m_dmesh;

	float* m_verts;
	int m_nverts;
	int* m_tris;
	int m_ntris;
	float* m_normals;
};

CRecastMesh *GetRecastNavMesh();

void InitRecastMesh();
void DestroyRecastMesh();

void LoadRecastMesh();
void ResetRecastMesh();

#endif // RECAST_MESH_H