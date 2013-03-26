//====== Copyright © 2013 Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "wars_mapboundary.h"
#include "collisionutils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Func version
IMPLEMENT_NETWORKCLASS_ALIASED( BaseFuncMapBoundary, DT_BaseFuncMapBoundary );

BEGIN_NETWORK_TABLE( CBaseFuncMapBoundary, DT_BaseFuncMapBoundary )
END_NETWORK_TABLE()

BEGIN_DATADESC( CBaseFuncMapBoundary )

	DEFINE_KEYFIELD( m_fBloat, FIELD_FLOAT, "Bloat"),	

END_DATADESC()

//LINK_ENTITY_TO_CLASS(info_map_boundary, CBaseMapBoundary);

// a list of map bounderies to search quickly
#ifndef CLIENT_DLL
CEntityClassList< CBaseFuncMapBoundary > g_FuncMapBounderiesList;
template< class CBaseFuncMapBoundary >
CBaseFuncMapBoundary *CEntityClassList< CBaseFuncMapBoundary >::m_pClassList = NULL;
#else
C_EntityClassList< CBaseFuncMapBoundary > g_FuncMapBounderiesList;
template< class CBaseFuncMapBoundary >
CBaseFuncMapBoundary *C_EntityClassList< CBaseFuncMapBoundary >::m_pClassList = NULL;
#endif

CBaseFuncMapBoundary *GetMapBoundaryList()
{
	return g_FuncMapBounderiesList.m_pClassList;
}

CBaseFuncMapBoundary::CBaseFuncMapBoundary()
{
	SetBlocksLOS( false );
	m_fBloat = -32.0f;
	g_FuncMapBounderiesList.Insert( this );
}

CBaseFuncMapBoundary::~CBaseFuncMapBoundary()
{
	g_FuncMapBounderiesList.Remove( this );
}


void CBaseFuncMapBoundary::GetMapBoundary( Vector &mins, Vector &maxs )
{
	CollisionProp()->WorldSpaceAABB( &mins, &maxs );

	mins.x -= m_fBloat;
	mins.y -= m_fBloat;
	mins.z -= m_fBloat;

	maxs.x += m_fBloat;
	maxs.y += m_fBloat;
	maxs.z += m_fBloat;
}

bool CBaseFuncMapBoundary::IsWithinMapBoundary( const Vector &vPoint, bool bIgnoreZ )
{
	Vector vAbsMins, vAbsMaxs;
	GetMapBoundary( vAbsMins, vAbsMaxs );
	if( bIgnoreZ )
	{
		vAbsMins.z = MIN_COORD_FLOAT;
		vAbsMaxs.z = MAX_COORD_FLOAT;
	}

	if( IsPointInBox( vPoint, vAbsMins, vAbsMaxs ) )
		return true;
	return false;
}

CBaseFuncMapBoundary *CBaseFuncMapBoundary::IsWithinAnyMapBoundary( const Vector &vPoint, bool bIgnoreZ )
{
	for( CBaseFuncMapBoundary *pEnt = GetMapBoundaryList(); pEnt != NULL; pEnt = pEnt->m_pNext )
	{
		if( pEnt && pEnt->IsWithinMapBoundary( vPoint, bIgnoreZ ) )
			return pEnt;
	}
	return NULL;
}

void CBaseFuncMapBoundary::SnapToNearestBoundary( Vector &vPoint, bool bUseMaxZ )
{
	Vector mins, maxs;
	float fNearestZ = MAX_COORD_FLOAT;
	vPoint.z = MAX( MIN_COORD_FLOAT, MIN( MAX_COORD_FLOAT, vPoint.z ) ); // Clamp within valid bounds

	for( CBaseFuncMapBoundary *pEnt = GetMapBoundaryList(); pEnt != NULL; pEnt = pEnt->m_pNext )
	{
		if( !pEnt )
			continue;

		if( pEnt->IsWithinMapBoundary( vPoint ) )
		{
			pEnt->GetMapBoundary(mins, maxs);
			vPoint.z = bUseMaxZ ? maxs.z : mins.z;
			//DevMsg("SnapToNearestBoundary: Modifying point z to %f\n", vPoint.z);
			break;
		}
		else if( pEnt->IsWithinMapBoundary( vPoint, true ) )
		{
			pEnt->GetMapBoundary(mins, maxs);
			float zdist = abs( mins.z - vPoint.z );
			if( zdist < fNearestZ )
			{
				fNearestZ = zdist;
				vPoint.z = bUseMaxZ ? maxs.z : mins.z;
				//DevMsg("SnapToNearestBoundary: Modifying point z to %f\n", vPoint.z);
			}
		}
	}
}