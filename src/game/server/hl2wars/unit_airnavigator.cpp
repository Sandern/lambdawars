//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "unit_airnavigator.h"
#include "unit_locomotion.h"
#include "hl2wars_util_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef DISABLE_PYTHON
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
UnitBaseAirNavigator::UnitBaseAirNavigator( boost::python::object outer )
	: UnitBaseNavigator(outer)
{
	m_iTestRouteMask = MASK_NPCSOLID_BRUSHONLY;
	m_bUseSimplifiedRouteBuilding = true;
}
#endif // DISABLE_PYTHON

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnitBaseAirNavigator::Update( UnitBaseMoveCommand &MoveCommand )
{
	trace_t tr;
	UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() - Vector(0, 0, MAX_TRACE_LENGTH), MASK_NPCSOLID_BRUSHONLY, 
		GetOuter(), GetOuter()->CalculateIgnoreOwnerCollisionGroup(), &tr);
	m_fCurrentHeight = GetAbsOrigin().z - tr.endpos.z;

	BaseClass::Update( MoveCommand );

	// Calculate upmove if needed
	MoveCommand.upmove = 0.0f;
	if( GetPath()->m_iGoalType != GOALTYPE_NONE && GetPath()->GetCurWaypoint() )
	{
		if( GetPath()->GetCurWaypoint()->GetPos().z + 32.0f > GetAbsOrigin().z )
		{
			// Calculate needed up movement
			/*MoveCommand.upmove = MIN(
				GetPath()->GetCurWaypoint()->GetPos().z - GetAbsOrigin().z,
				MoveCommand.maxspeed
			);*/

			// Zero out other movement (so we don't bump into a cliff or wall)
			if( m_LastGoalStatus == CHS_CLIMBDEST )
				MoveCommand.forwardmove = MoveCommand.sidemove = 0.0f;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CheckGoalStatus_t UnitBaseAirNavigator::MoveUpdateWaypoint()
{
	UnitBaseWaypoint *pCurWaypoint = GetPath()->m_pWaypointHead;
	if( pCurWaypoint->SpecialGoalStatus == CHS_CLIMBDEST )
	{
		// Must be at the right height! The regular check is only 2D.
		if( GetPath()->GetCurWaypoint()->GetPos().z + 4.0f > GetAbsOrigin().z )
			return pCurWaypoint->SpecialGoalStatus;
	}
	return BaseClass::MoveUpdateWaypoint();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool UnitBaseAirNavigator::TestRoute( const Vector &vStartPos, const Vector &vEndPos )
{
	Vector vStart = vStartPos;
	vStart.z += m_fCurrentHeight;
	Vector vEnd = vEndPos;
	vEnd.z += m_fCurrentHeight;

	//CTraceFilterSkipFriendly filter( GetOuter(), GetOuter()->CalculateIgnoreOwnerCollisionGroup(), GetOuter() );
	//CTraceFilterSimple filter( GetOuter(), GetOuter()->CalculateIgnoreOwnerCollisionGroup() );
	CTraceFilterWorldOnly filter;
	trace_t tr;
	UTIL_TraceHull( vStart, vEnd, WorldAlignMins(), WorldAlignMaxs(), m_iTestRouteMask, 
		&filter, &tr);
	//NDebugOverlay::Line( vStart, tr.endpos, 255, 0, 0,  true, 1.0f );
	if( tr.DidHit() )
		return false;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
UnitBaseWaypoint *UnitBaseAirNavigator::BuildLocalPath( const Vector &vGoalPos )
{
	if( !m_bUseSimplifiedRouteBuilding || GetBlockedStatus() >= BS_LITTLE )
	{
		// Do a simple trace, always do this for air units
		trace_t tr;
		CTraceFilterWorldOnly filter;
		UTIL_TraceHull( GetAbsOrigin(), vGoalPos, WorldAlignMins(), WorldAlignMaxs(), m_iTestRouteMask, 
			&filter, &tr);
		//UTIL_TraceHull(GetAbsOrigin()+Vector(0,0,16.0), vGoalPos+Vector(0,0,16.0), 
		//	WorldAlignMins(), WorldAlignMaxs(), MASK_SOLID, GetOuter(), GetOuter()->CalculateIgnoreOwnerCollisionGroup(), &tr);
		if( tr.DidHit() && (!GetPath()->m_hTarget || !tr.m_pEnt || tr.m_pEnt != GetPath()->m_hTarget) )
			return NULL;

		NavDbgMsg("#%d BuildLocalPath: builded local route\n", GetOuter()->entindex());
		return new UnitBaseWaypoint(vGoalPos);
	}
	else
	{
		NavDbgMsg("#%d BuildLocalPath: builded direct route\n", GetOuter()->entindex());
		return new UnitBaseWaypoint(vGoalPos);
	}
}