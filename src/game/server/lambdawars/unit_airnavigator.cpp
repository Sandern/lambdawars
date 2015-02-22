//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "unit_airnavigator.h"
#include "unit_locomotion.h"
#include "hl2wars_util_shared.h"

#include "recast/recast_mgr.h"
#include "recast/recast_mesh.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef ENABLE_PYTHON
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
UnitBaseAirNavigator::UnitBaseAirNavigator( boost::python::object outer )
	: UnitBaseNavigator(outer)
{
	m_iTestRouteMask = MASK_NPCSOLID_BRUSHONLY;
	m_bUseSimplifiedRouteBuilding = true;
	m_bTestRouteWorldOnly = true;
	m_fCurrentHeight = m_fDesiredHeight = 0.0f;
}
#endif // ENABLE_PYTHON

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnitBaseAirNavigator::Update( UnitAirMoveCommand &MoveCommand )
{
	// Get reported height from locomotion
	m_fCurrentHeight = MoveCommand.height;
	m_fDesiredHeight = MoveCommand.desiredheight;

	bool bIsAtMinDesiredHeight = m_fCurrentHeight >= m_fDesiredHeight;

	BaseClass::Update( MoveCommand );

	// Calculate upmove if needed
	MoveCommand.upmove = 0.0f;
	if( GetPath()->m_iGoalType != GOALTYPE_NONE && GetPath()->GetCurWaypoint() )
	{
		bool bCurTargetIsGoal = (GetPath()->m_iGoalType == GOALTYPE_TARGETENT || GetPath()->m_iGoalType == GOALTYPE_TARGETENT_INRANGE) && GetPath()->CurWaypointIsGoal();

		float fTargetZ = GetPath()->GetCurWaypoint()->GetPos().z + (-GetOuter()->CollisionProp()->OBBMins().z);
		if( m_LastGoalStatus == CHS_CLIMBDEST || !bCurTargetIsGoal )
			fTargetZ += m_fDesiredHeight;

		if( m_LastGoalStatus == CHS_CLIMBDEST || bCurTargetIsGoal )
		{
			if( fTargetZ > GetAbsOrigin().z )
			{
				// Calculate needed up movement
				MoveCommand.upmove = Max( -MoveCommand.maxspeed, Min(
					(fTargetZ - GetAbsOrigin().z) / MoveCommand.interval,
					MoveCommand.maxspeed
				) );

				// Zero out other movement (so we don't bump into a cliff or wall)
				if( m_LastGoalStatus == CHS_CLIMBDEST )
					MoveCommand.forwardmove = MoveCommand.sidemove = 0.0f;
			}
			else if( fTargetZ > GetAbsOrigin().z && bIsAtMinDesiredHeight )
			{
				// Avoid going down again
				MoveCommand.upmove = 1.0f;
			}
		}
		else if( fTargetZ > GetAbsOrigin().z && bIsAtMinDesiredHeight )
		{
			// Avoid going down again to prevent navigation issues
			MoveCommand.upmove = 1.0f;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CheckGoalStatus_t UnitBaseAirNavigator::MoveUpdateWaypoint( UnitBaseMoveCommand &MoveCommand )
{
	UnitBaseWaypoint *pCurWaypoint = GetPath()->m_pWaypointHead;
	if( pCurWaypoint->SpecialGoalStatus == CHS_CLIMBDEST )
	{
		// Must be at the right height! The regular check is only 2D.
		if( GetPath()->GetCurWaypoint()->GetPos().z + 4.0f > GetAbsOrigin().z )
			return pCurWaypoint->SpecialGoalStatus;
	}
	return BaseClass::MoveUpdateWaypoint( MoveCommand );
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

		if( m_bTestRouteWorldOnly )
		{
			CTraceFilterWorldOnly filter;
			UTIL_TraceHull( GetAbsOrigin(), vGoalPos, WorldAlignMins(), WorldAlignMaxs(), m_iTestRouteMask, 
				&filter, &tr);
		}
		else
		{
			CTraceFilterSimple filter( GetOuter(), WARS_COLLISION_GROUP_IGNORE_ALL_UNITS );
			UTIL_TraceHull( GetAbsOrigin(), vGoalPos, WorldAlignMins(), WorldAlignMaxs(), m_iTestRouteMask, 
				&filter, &tr);
		}

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

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
extern ConVar unit_route_navmesh_paths;
UnitBaseWaypoint *UnitBaseAirNavigator::BuildNavAreaPath( UnitBasePath *pPath, const Vector &vGoalPos )
{
	if( !unit_route_navmesh_paths.GetBool() )
		return NULL;

	CRecastMesh *pNavMesh = GetNavMesh();
	if( pNavMesh )
	{
		const Vector &vStart = GetAbsOrigin();
		UnitBaseWaypoint *pFoundPath = pNavMesh->FindPath( vStart, vGoalPos, 500.0f, pPath->GetTarget() );
		if( pFoundPath )
			return pFoundPath;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRecastMesh *UnitBaseAirNavigator::GetNavMesh()
{
	// Make something better, but for now prefer the regular mesh for units flying below 100 units.
	// Mainly intended for manhacks.
	if( m_fCurrentHeight != 0 && m_fCurrentHeight < 100.0f )
	{
		CRecastMesh *pMesh = BaseClass::GetNavMesh();
		if( pMesh )
			return pMesh;
	}

	int idx = RecastMgr().FindMeshIndex( "air" );
	return RecastMgr().GetMesh( idx );
}