//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "potential_field.h"

#include "unit_base_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static CPotentialFieldMgr s_PotentialFieldMgr; // singleton

CPotentialFieldMgr* PotentialFieldMgr() { return &s_PotentialFieldMgr; }

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPotentialField::Reset()
{
	memset(m_Density, 0, sizeof(float)*(PF_ARRAYSIZE)*(PF_ARRAYSIZE));
	memset(m_AvgVelocities, 0, sizeof(Vector)*(PF_ARRAYSIZE)*(PF_ARRAYSIZE));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
extern float gaussian( float x, float y, float s );
extern ConVar unit_potential_stdev;
void CPotentialField::Update()
{
	int iPosX, iPosY, iMinX, iMinY, iMaxX, iMaxY, x, y;
	CUnitBase *pUnit;
	CUnitBase **ppUnits = g_Unit_Manager.AccessUnits();
	int i;
	for ( i = 0; i < g_Unit_Manager.NumUnits(); i++ )
	{
		pUnit = ppUnits[i];

		if( !pUnit->IsSolid() || !pUnit->CanBeSeen() )
			continue;

		const Vector &origin = pUnit->GetAbsOrigin();

		iPosX = (origin.x + (PF_SIZE / 2)) / (float)PF_TILESIZE;
		iPosY = (origin.y + (PF_SIZE / 2)) / (float)PF_TILESIZE;

		iMinX = MAX(iPosX-PF_CONSIDER, 0);
		iMaxX = MIN(PF_SIZE-1, iPosX+PF_CONSIDER);
		iMinY = MAX(iPosY-PF_CONSIDER, 0);
		iMaxY = MIN(PF_SIZE-1, iPosY+PF_CONSIDER);

		for(x=iMinX; x <= iMaxX; x++)
		{
			for(y=iMinY; y <= iMaxY; y++)
			{
				m_Density[x][y] += gaussian(x-iPosX, y-iPosY, unit_potential_stdev.GetFloat());
				m_AvgVelocities[x][y] += pUnit->GetAbsVelocity()*m_Density[x][y];
			}
		}
	};

	for(x=0; x<PF_ARRAYSIZE; x++)
	{
		for(y=0; y<PF_ARRAYSIZE; y++)
		{
			if( m_Density[x][y] != 0 )
				m_AvgVelocities[x][y] /=  m_Density[x][y];
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPotentialFieldMgr::CPotentialFieldMgr()
{
	m_pPotentialField = new CPotentialField();
	m_bActive = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPotentialFieldMgr::LevelInitPostEntity()
{
	//m_bActive = true;
	m_fNeedsUpdate = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPotentialFieldMgr::LevelShutdownPostEntity()
{
	m_bActive = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CPotentialFieldMgr::NeedsUpdate()
{
	if( m_fNeedsUpdate < gpGlobals->curtime )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPotentialFieldMgr::FrameUpdatePreEntityThink()
{
	VPROF_BUDGET( "CPotentialFieldMgr::FrameUpdatePreEntityThink", VPROF_BUDGETGROUP_POTENTIALFIELD );

	if( !m_bActive )
		return;

	if( !NeedsUpdate() )
		return;

	m_fNeedsUpdate = gpGlobals->curtime + 0.1f;

	m_pPotentialField->Reset();
	m_pPotentialField->Update();
}