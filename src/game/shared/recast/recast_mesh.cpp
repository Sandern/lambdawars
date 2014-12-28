//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "recast_mesh.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar recast_edit( "recast_edit", "0", FCVAR_GAMEDLL | FCVAR_CHEAT, "Set to one to interactively edit the Recast Navigation Mesh. Set to zero to leave edit mode." );

static CRecastMesh *s_pRecastMesh = NULL;

class BuildContext : public rcContext
{
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRecastMesh::CRecastMesh()
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
bool CRecastMesh::Load()
{
	BuildContext ctx;

	//
	// Step 1. Initialize build config.
	//
	
	// Init build configuration from GUI
	memset(&m_cfg, 0, sizeof(m_cfg));
	m_cfg.cs = 25.0f;
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
	m_cfg.detailSampleMaxError = 100 * 1;

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
		Warning("buildNavigation: Out of memory 'solid'.\n");
		return false;
	}
	if (!rcCreateHeightfield(&ctx, *m_solid, m_cfg.width, m_cfg.height, m_cfg.bmin, m_cfg.bmax, m_cfg.cs, m_cfg.ch))
	{
		Warning("buildNavigation: Could not create solid heightfield.\n");
		return false;
	}
	
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CRecastMesh::Reset()
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Draws the mesh
//-----------------------------------------------------------------------------
void CRecastMesh::DebugRender()
{
	if( !recast_edit.GetBool() )
		return;
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
