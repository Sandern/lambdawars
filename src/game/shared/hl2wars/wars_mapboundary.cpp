//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "wars_mapboundary.h"
#include "collisionutils.h"
#include "hl2wars_util_shared.h"

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

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseFuncMapBoundary *GetMapBoundaryList()
{
	return g_FuncMapBounderiesList.m_pClassList;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseFuncMapBoundary::CBaseFuncMapBoundary()
{
	SetBlocksLOS( false );
	m_fBloat = -32.0f;
	g_FuncMapBounderiesList.Insert( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseFuncMapBoundary::~CBaseFuncMapBoundary()
{
	g_FuncMapBounderiesList.Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseFuncMapBoundary::IsWithinMapBoundary( const Vector &vPoint, const Vector &vMins, const Vector &vMaxs, bool bIgnoreZ )
{
	trace_t tr;
	CTraceFilterWars traceFilter( NULL, COLLISION_GROUP_PLAYER_MOVEMENT );
	UTIL_TraceHull( vPoint, GetAbsOrigin(), vMins, vMaxs, CONTENTS_PLAYERCLIP, &traceFilter, &tr );
	return tr.fraction == 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseFuncMapBoundary *CBaseFuncMapBoundary::IsWithinAnyMapBoundary( const Vector &vPoint, const Vector &vMins, const Vector &vMaxs, bool bIgnoreZ )
{
	for( CBaseFuncMapBoundary *pEnt = GetMapBoundaryList(); pEnt != NULL; pEnt = pEnt->m_pNext )
	{
		if( pEnt && pEnt->IsWithinMapBoundary( vPoint, vMins, vMaxs, bIgnoreZ ) )
			return pEnt;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseFuncMapBoundary::SnapToNearestBoundary( Vector &vPoint, const Vector &vMins, const Vector &vMaxs, bool bUseMaxZ )
{
	trace_t tr;
	CTraceFilterWars traceFilter( NULL, COLLISION_GROUP_PLAYER_MOVEMENT );

	Vector mins, maxs;
	vPoint.z = Max( MIN_COORD_FLOAT, Min( MAX_COORD_FLOAT, vPoint.z ) ); // Clamp within valid bounds

	float fBestDist = MAX_COORD_FLOAT * MAX_COORD_FLOAT;

	for( CBaseFuncMapBoundary *pEnt = GetMapBoundaryList(); pEnt != NULL; pEnt = pEnt->m_pNext )
	{
		if( !pEnt )
			continue;

		UTIL_TraceHull( pEnt->GetAbsOrigin(), vPoint, vMins, vMaxs, CONTENTS_PLAYERCLIP, &traceFilter, &tr );

		float fDist = ( pEnt->GetAbsOrigin() - tr.endpos ).Length2DSqr();
		if( fDist < fBestDist )
		{
			vPoint = tr.endpos;
			fBestDist = fDist;
		}
	}

	if( bUseMaxZ )
		UTIL_TraceHull( vPoint, vPoint + Vector(0, 0, COORD_EXTENT), vMins, vMaxs, CONTENTS_PLAYERCLIP, &traceFilter, &tr );
	else
		UTIL_TraceHull( vPoint, vPoint - Vector(0, 0, COORD_EXTENT), vMins, vMaxs, CONTENTS_PLAYERCLIP, &traceFilter, &tr );
	vPoint = tr.endpos;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseFuncMapBoundary::DidHitMapBoundary( CBaseEntity *pHitEnt )
{
	if( !pHitEnt )
		return false;

	for( CBaseFuncMapBoundary *pEnt = GetMapBoundaryList(); pEnt != NULL; pEnt = pEnt->m_pNext )
	{
		if( pEnt == pHitEnt )
			return true;
	}

	return false;
}