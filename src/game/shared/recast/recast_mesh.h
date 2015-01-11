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

class UnitBaseWaypoint;

class CRecastMesh
{
public:
	CRecastMesh();
	~CRecastMesh();

	const char *GetName();
	void SetName( const char *name );

	// Load/build 
	//virtual void LoadTestData();
	virtual void ResetCommonSettings();

	virtual bool Build();
	virtual bool Load( CUtlBuffer &fileBuffer );
	virtual bool Reset();

#ifndef CLIENT_DLL
	virtual bool Save( CUtlBuffer &fileBuffer );
#endif // CLIENT_DLL

#ifdef CLIENT_DLL
	virtual void DebugRender();
#endif // CLIENT_DLL

#ifndef CLIENT_DLL
	// Path find functions
	virtual UnitBaseWaypoint *FindPath( const Vector &vStart, const Vector &vEnd );
#endif // CLIENT_DLL

public:

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
	CUtlString m_Name;
	
	// Data used during build
	unsigned char* m_triareas;
	rcHeightfield* m_solid;
	rcCompactHeightfield* m_chf;
	rcContourSet* m_cset;
	rcPolyMesh* m_pmesh;
	rcConfig m_cfg;	
	rcPolyMeshDetail* m_dmesh;

	// Data used for path finding
	class dtNavMesh* m_navMesh;
	class dtNavMeshQuery* m_navQuery;
};

inline const char *CRecastMesh::GetName()
{
	return m_Name.Get();
}

inline void CRecastMesh::SetName( const char *name )
{
	m_Name.Set( name );
}

#endif // RECAST_MESH_H