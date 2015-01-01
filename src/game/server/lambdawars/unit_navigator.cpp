//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: The unit navigator consist of two major components:
//			1. Building a global path using the Navigation Mesh
//			2. Local avoidance of units, buildings and objects.
//
//			The main routine is the Update function. This will update the move command class.
//			This contains a forward and right move value, similar to how players are controlled.
//			This makes it easy to switch between a player or navigator controlling an unit.
//
// Pathfinding: Query the navigation mesh for a path. Depending on the unit settings
//				a different path is generated (drop down height, climb/jump support, etc).
//				SetGoal can be used to set a new path. Furthermore the current path can be
//				saved in Python by storing the path object. This can then be restored using
//				SetPath.
//
// Local Obstacle Avoidance + Goal Updating:
//			Because of the high number of units this is done using density/flow fields.
//			A full update looks as follows:
//			1. Do one trace/query to determine the nearby units. No other traces are done in the navigator.
//			   Then process the entity list, filtering out entities.
//			2. Update Goal and Path
//					1. Check if goal is valid. For example the target ent might have gone NULL.
//					2. Check if we cleared a waypoint. In case the waypoint is cleared, check if we arrived
//						at our goal or if it's a special waypoint (climbing, jumping, etc) dispatch an event.
//				    3. If not a special waypoint, do a reactive path update (see if we can skip waypoints).
//					   This results in a more smoothed path.
//			3. (IMPORTANT) Compute the desired move velocity taking the path velocity and nearby units into account.
//				Compute the flow velocity depending on the nearby units. 
//				This is final velocity depends on how close you are to other objects.
//				If you are very close the density will be high and your velocity will be dominated by the flow.
//				This is based on the work of "Continuum Crowds" (http://grail.cs.washington.edu/projects/crowd-flows/).
//			4. Updated ideal angles. Face path, enemy, specific angle or nothing at all.
//			5. Update move command based on computed velocity.
//			6. Dispatch nav complete or failed depending on the goal status (if needed). Always done at the end of the Update.
//
// Additional Notes:
// - During route testing units use their eye offset to tweak their current test position. Make sure this is a good value!
//	(otherwise it can result in blocked movement, even though the unit is not).
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "unit_navigator.h"
#include "unit_baseanimstate.h"
#include "hl2wars_util_shared.h"
#include "fowmgr.h"

#include "nav_mesh.h"
#include "nav_pathfind.h"
#include "hl2wars_nav_pathfind.h"

#include "recast/recast_mesh.h"

#ifdef ENABLE_PYTHON
	#include "srcpy.h"
#endif // ENABLE_PYTHON

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Settings
ConVar unit_reactivepath("unit_reactivepath", "0", 0, "Optimize the current path each update."); // TODO: Update for recast mesh
ConVar unit_reactivepath_maxlookahead("unit_reactivepath_maxlookahead", "2048.0", 0, "Max distance a path is optimized each update.");
ConVar unit_reactivepath_maxwaypointsahead("unit_reactivepath_maxwaypointsahead", "5", 0, "Max number of waypoints being looked ahead.");
ConVar unit_nonavigator("unit_nonavigator", "0", 0, "Do not perform navigation");
ConVar unit_navigator_eattest("unit_navigator_eattest", "0", 0, "Perform navigation, but do not output the calculated move values.");
ConVar unit_route_requirearea("unit_route_requirearea", "1", 0, "Only try to build a route though the nav mesh if a start and goal area can be found");

ConVar unit_consider_multiplier("unit_consider_multiplier", "2.5", 0, "Multiplies the distance used for computing the entity consider list. The base distance is the unit 2D bounding radius.");

ConVar unit_potential_tmin_offset("unit_potential_tmin_offset", "0.0");
ConVar unit_potential_nogoal_tmin_offset("unit_potential_nogoal_tmin_offset", "0.0");
ConVar unit_potential_tmax("unit_potential_tmax", "1.0");
ConVar unit_potential_nogoal_tmax("unit_potential_nogoal_tmax", "0.0");
ConVar unit_potential_threshold("unit_potential_threshold", "0.0009");

ConVar unit_dens_all_nomove("unit_dens_all_nomove", "0.5");

ConVar unit_cost_distweight("unit_cost_distweight", "1.0");
ConVar unit_cost_timeweight("unit_cost_timeweight", "1.0");
ConVar unit_cost_blockdirweight("unit_cost_blockdirweight", "75.0");
ConVar unit_cost_discomfortweight_start("unit_cost_discomfortweight_start", "1.0");
ConVar unit_cost_discomfortweight_growthreshold("unit_cost_discomfortweight_growthreshold", "0.05");
ConVar unit_cost_discomfortweight_growrate("unit_cost_discomfortweight_growrate", "500.0");
ConVar unit_cost_discomfortweight_max("unit_cost_discomfortweight_max", "25000.0");
ConVar unit_cost_history("unit_cost_history", "0.2");
ConVar unit_cost_minavg_improvement("unit_cost_minavg_improvement", "0.0");
ConVar unit_cost_nonavareacheck("unit_cost_nonavareacheck", "0", FCVAR_CHEAT, "");

ConVar unit_nogoal_mindiff("unit_nogoal_mindiff", "0.25");
ConVar unit_nogoal_mindest("unit_nogoal_mindest", "0.4");

ConVar unit_testroute_stepsize("unit_testroute_stepsize", "48.0");
ConVar unit_testroute_bloatscale("unit_testroute_bloatscale", "1.2");

ConVar unit_seed_radius_bloat("unit_seed_radius_bloat", "2.0");
ConVar unit_seed_density("unit_seed_density", "0.12");
ConVar unit_seed_historytime("unit_seed_historytime", "0.5");

ConVar unit_allow_cached_paths("unit_allow_cached_paths", "1");
ConVar unit_route_local_paths("unit_route_local_paths", "1");
ConVar unit_route_navmesh_paths("unit_route_navmesh_paths", "1");

ConVar unit_blocked_lowvel_check("unit_blocked_lowvel_check", "1");
ConVar unit_blocked_longdist_check("unit_blocked_longdist_check", "1");

ConVar unit_navigator_debug("unit_navigator_debug", "0", 0, "Prints debug information about the unit navigator");
ConVar unit_navigator_debug_inrange("unit_navigator_debug_inrange", "0", 0, "Prints debug information for in range checks");
ConVar unit_navigator_debug_show("unit_navigator_debug_show", "0", 0, "Shows debug information about the unit navigator");
ConVar unit_navigator_debugoverlay_ent("unit_navigator_debugoverlay_ent", "-1", 0, "Shows debug overlay information only for specific unit");
ConVar unit_navigator_debugscreen_ent("unit_navigator_debugscreen_ent", "-1", 0, "Shows debug screen information only for specific unit");
ConVar unit_navigator_notestnearbyunits("unit_navigator_notestnearbyunits", "0", 0, "Don't test nearby units for same goal");
ConVar unit_navigator_debug_showavgvels("unit_navigator_debug_showavgvels", "0", 0, "");

#define THRESHOLD unit_potential_threshold.GetFloat()
#define THRESHOLD_MIN (THRESHOLD+(GetPath()->m_iGoalType != GOALTYPE_NONE ? unit_potential_tmin_offset.GetFloat() : unit_potential_nogoal_tmin_offset.GetFloat()) )
#define THRESHOLD_MAX (GetPath()->m_iGoalType != GOALTYPE_NONE ? unit_potential_tmax.GetFloat() : unit_potential_nogoal_tmax.GetFloat())

float UnitComputePathDirection( const Vector &start, const Vector &end, Vector &pDirection );

int UnitBaseNavigator::m_iCurPathRecomputations = 0;

//-----------------------------------------------------------------------------
// Purpose: Alternative path direction function that computes the direction using
//			the nav mesh. It picks the direction to the closest point on the portal
//			of the next target nav area.
//-----------------------------------------------------------------------------
float UnitComputePathDirection2( const Vector &start, UnitBaseWaypoint *pEnd, Vector &pDirection )
{
	if( pEnd->navDir == NUM_DIRECTIONS )
	{
		return UnitComputePathDirection( start, pEnd->GetPos(), pDirection );
	}

	Vector dir, point1, point2, end;
	//DirectionToVector2D( DirectionLeft(pEnd->navDir), &(dir.AsVector2D()) );
	//dir.z = 0.0f;
	dir = pEnd->areaSlope;

	if( pEnd->navDir == WEST || pEnd->navDir == EAST )
	{
		point1 = pEnd->GetPos() + dir * pEnd->flToleranceX;
		point2 = pEnd->GetPos() + -1 * dir * pEnd->flToleranceX;
	}
	else
	{		point1 = pEnd->GetPos() + dir * pEnd->flToleranceY;
		point2 = pEnd->GetPos() + -1 * dir * pEnd->flToleranceY;
	}

	if( point1 == point2 )
		return UnitComputePathDirection( start, pEnd->GetPos(), pDirection );

	end = UTIL_PointOnLineNearestPoint( point1, point2, start, true ); 

	if( unit_navigator_debug.GetInt() == 2 )
	{
		NDebugOverlay::Line( point1, point2, 0, 255, 0, true, 1.0f );
		NDebugOverlay::Box( end, -Vector(8, 8, 8), Vector(8, 8, 8), 255, 255, 0, true, 1.0f );
	}
	
	VectorSubtract( end, start, pDirection );
	pDirection.z = 0.0f;
	return Vector2DNormalize( pDirection.AsVector2D() );
}

//-------------------------------------

UnitBaseWaypoint *	UnitBaseWaypoint::GetLast()
{
	Assert( !pNext || pNext->pPrev == this ); 
	UnitBaseWaypoint *pCurr = this;
	while (pCurr->GetNext())
	{
		pCurr = pCurr->GetNext();
	}

	return pCurr;
}

#ifdef ENABLE_PYTHON
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
UnitBaseNavigator::UnitBaseNavigator( boost::python::object outer )
		: UnitComponent(outer)
{
	SetPath( boost::python::object() ); // Create initial path object
	Reset();

	m_fIdealYaw = -1;
	m_fIdealYawTolerance = 2.5f;
	m_hFacingTarget = NULL;
	m_fFacingCone = 0.7;
	m_vFacingTargetPos = vec3_origin;
	m_bFacingFaceTarget = false;
	m_bNoAvoid = false;
	m_bNoNavAreasNearby = false;
	m_fNextAllowPathRecomputeTime = 0.0f;
	m_bLimitPositionActive = false;
	m_iTestRouteMask = MASK_SOLID;
}
#endif // ENABLE_PYTHON

//-----------------------------------------------------------------------------
// Purpose: Clear variables
//-----------------------------------------------------------------------------
void UnitBaseNavigator::Reset()
{
	if( unit_navigator_debug.GetBool() )
		DevMsg( "#%d UnitNavigator: reset status\n", m_pOuter->entindex());

	m_LastGoalStatus = CHS_NOGOAL;
	m_vForceGoalVelocity.Invalidate();
	m_fGoalDistance = -1;
	m_bNoPathVelocity = false;
	m_fIgnoreNavMeshTime = 0.0f;
	m_bNoNavAreasNearby = true; // Reset, so we ignore nav meshes until we found a valid area again

	m_vLastWishVelocity.Init(0, 0, 0);

	m_fLastPathRecomputation = 0.0f;
	m_fNextReactivePathUpdate = 0.0f;
	m_iLastTargetArea = 0;
	ResetBlockedStatus();

	m_fNextAvgDistConsideration = gpGlobals->curtime + unit_cost_history.GetFloat();
	m_fLastAvgDist = -1;

	m_fLastBestDensity = 0.0f;
	m_fDiscomfortWeight = unit_cost_discomfortweight_start.GetFloat();

	m_iBlockResolveDirection = 1;
	m_fNextChooseDirection = 0;

	m_hAtGoalDependencyEnt = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnitBaseNavigator::StopMoving()
{
#ifdef ENABLE_PYTHON
	boost::python::object path = m_refPath;
	UnitBasePath *pPath = m_pPath;
	GetPath()->m_iGoalType = GOALTYPE_NONE; // Just clear goal, so we can reuse it if needed

	GetPath()->m_vGoalPos = pPath-> m_vGoalPos;
#endif // ENABLE_PYTHON
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnitBaseNavigator::DispatchOnNavComplete()
{
	if( unit_navigator_debug.GetBool() )
		DevMsg("#%d UnitNavigator: At goal. Dispatching success (OnNavComplete).\n", GetOuter()->entindex());

	// Keep path around for querying the information about the last path
	GetPath()->m_iGoalType = GOALTYPE_NONE;
	GetPath()->m_bSuccess = true;
	Reset();

#ifdef ENABLE_PYTHON
	SrcPySystem()->Run<const char *>( 
		SrcPySystem()->Get("DispatchEvent", GetOuter()->GetPyInstance() ), 
		"OnNavComplete"
	);
#endif // ENABLE_PYTHON
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnitBaseNavigator::DispatchOnNavFailed()
{
	// Keep path around for querying the information about the last path
	GetPath()->m_iGoalType = GOALTYPE_NONE; 
	Reset();

#ifdef ENABLE_PYTHON
	SrcPySystem()->Run<const char *>( 
		SrcPySystem()->Get("DispatchEvent", GetOuter()->GetPyInstance() ), 
		"OnNavFailed"
	);
#endif // ENABLE_PYTHON
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnitBaseNavigator::DispatchOnNavAtGoal()
{
	// Reset blocked status, otherwise we might get an instant fail if it changes back to has goal
	ResetBlockedStatus();

	if( GetPath()->m_iGoalFlags & GF_ATGOAL_RELAX )
		GetPath()->m_fAtGoalTolerance = m_fGoalDistance + GetEntityBoundingRadius(m_pOuter) * 4.5f;

 	GetPath()->m_bSuccess = true;

	if( unit_navigator_debug.GetBool() )
		DevMsg("#%d UnitNavigator: At goal, but marked as \"no clear\". Dispatching success one time (OnNavAtGoal).\n", GetOuter()->entindex());

#ifdef ENABLE_PYTHON
	SrcPySystem()->Run<const char *>( 
		SrcPySystem()->Get("DispatchEvent", GetOuter()->GetPyInstance() ), 
		"OnNavAtGoal"
	);		
#endif // ENABLE_PYTHON
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnitBaseNavigator::DispatchOnNavLostGoal()
{
	GetPath()->m_bSuccess = false;

	if( unit_navigator_debug.GetBool() )
		DevMsg("#%d UnitNavigator: Was at goal, but lost it. Dispatching lost (OnNavLostGoal).\n", GetOuter()->entindex());

#ifdef ENABLE_PYTHON
	SrcPySystem()->Run<const char *>( 
		SrcPySystem()->Get("DispatchEvent", GetOuter()->GetPyInstance() ), 
		"OnNavLostGoal"
	);		
#endif // ENABLE_PYTHON
}

//-----------------------------------------------------------------------------
// Purpose: Main routine. Update the movement variables.
//-----------------------------------------------------------------------------
void UnitBaseNavigator::Update( UnitBaseMoveCommand &MoveCommand )
{
	VPROF_BUDGET( "UnitBaseNavigator::Update", VPROF_BUDGETGROUP_UNITS );

	if( unit_nonavigator.GetBool() )
		return;

	// Check goal and update path
	Vector vPathDir;
	float fWaypointDist;
	CheckGoalStatus_t GoalStatus;

	// Allow the AI to override the desired goal velocity
	if( m_vForceGoalVelocity.IsValid() )
	{
		vPathDir = m_vForceGoalVelocity;
		fWaypointDist = VectorNormalize( vPathDir ) + 1000.0f;
		GoalStatus = CHS_HASGOAL;
		RegenerateConsiderList( MoveCommand, vPathDir, GoalStatus );
	}
	else
	{
		GoalStatus = UpdateGoalAndPath( MoveCommand, vPathDir, fWaypointDist );
	}

	// Compute velocity
	if( GoalStatus != CHS_ATGOAL || (GetPath()->m_iGoalFlags & GF_ATGOAL_RELAX) )
	{
		if( !m_bNoAvoid || GoalStatus == CHS_NOGOAL )
		{
			// Compute our wish velocity based on the flow velocity and path velocity (if any)
			m_vLastWishVelocity = ComputeVelocity( GoalStatus, MoveCommand, vPathDir, fWaypointDist );
		}
		else
		{
			float fMaxTravelDist = MoveCommand.maxspeed * MoveCommand.interval;

			// Desire full speed when not near the final goal
			if( (fWaypointDist-MoveCommand.stopdistance) <= fMaxTravelDist )
				m_vLastWishVelocity = ((fWaypointDist-MoveCommand.stopdistance)/MoveCommand.interval) * vPathDir;
			else
				m_vLastWishVelocity = MoveCommand.maxspeed * vPathDir;
		}
	}
	else
	{
		m_vLastWishVelocity.Zero();
	}

	// Finally update the move command
	float fSpeed;
	Vector vDir;

	vDir = m_vLastWishVelocity;
	fSpeed = VectorNormalize(vDir);

	UpdateIdealAngles( MoveCommand, (GoalStatus == CHS_HASGOAL) ? &vPathDir : NULL );

	QAngle vAngles;
	VectorAngles( vDir, vAngles );
	CalcMove( MoveCommand, vAngles, fSpeed );

	UpdateBlockedStatus( MoveCommand, fWaypointDist );
	if( GoalStatus == CHS_HASGOAL && GetBlockedStatus() == BS_GIVEUP )
	{
		Warning("#%d UnitNavigator: Unit gives up on goal due being blocked!\n", GetOuter()->entindex());
		GoalStatus = CHS_FAILED;
	}

	UpdateGoalStatus( MoveCommand, GoalStatus );

	if( unit_navigator_debug_show.GetBool() )
	{
		static float sNextUpdateNavStats = 0.0f;
		if( sNextUpdateNavStats < gpGlobals->curtime )
		{
			engine->Con_NPrintf( 1, "Path Recomputations last second: %d", m_iCurPathRecomputations );

			// Reset
			sNextUpdateNavStats = gpGlobals->curtime + 1.0f;
			m_iCurPathRecomputations = 0;
		}
	}

	if( unit_navigator_debugscreen_ent.GetInt() == GetOuter()->entindex() )
	{
		int offset = 2;
		engine->Con_NPrintf( offset++, "Goal status: %d", m_LastGoalStatus );
		engine->Con_NPrintf( offset++, "Blocked status: %d", GetBlockedStatus() );
		engine->Con_NPrintf( offset++, "WishVel: %f %f %f (%f)", m_vLastWishVelocity.x, m_vLastWishVelocity.y, m_vLastWishVelocity.z, m_vLastWishVelocity.Length() );
		engine->Con_NPrintf( offset++, "Velocity: %f %f %f (%f)", GetAbsVelocity().x, GetAbsVelocity().y, GetAbsVelocity().z, GetAbsVelocity().Length() );
		engine->Con_NPrintf( offset++, "m_bBlockedLongDistanceDetected: %d", m_bBlockedLongDistanceDetected );
		engine->Con_NPrintf( offset++, "m_fLastWaypointDistance: %f", m_fLastWaypointDistance );
		engine->Con_NPrintf( offset++, "m_bLowVelocityDetectionActive: %d", m_bLowVelocityDetectionActive );
		engine->Con_NPrintf( offset++, "m_bNoPathVelocity: %d", m_bNoPathVelocity );
		engine->Con_NPrintf( offset++, "m_bLimitPositionActive: %d", m_bLimitPositionActive );
		engine->Con_NPrintf( offset++, "(gpGlobals->curtime - m_fLowVelocityStartTime) < 1.0f: %d", (gpGlobals->curtime - m_fLowVelocityStartTime) < 1.0f );
	}
}

//-----------------------------------------------------------------------------
//  
//-----------------------------------------------------------------------------
void UnitBaseNavigator::UpdateGoalStatus( UnitBaseMoveCommand &MoveCommand, CheckGoalStatus_t GoalStatus )
{
	// Move dispatch complete/failed to the end of the Update in case we are at the goal
	// This way the event can clear the move command if it wants
	// It's also more clear to keep all event dispatching here, since they might result in a new
	// goal being set.
	CheckGoalStatus_t LastGoalStatus = m_LastGoalStatus;
	m_LastGoalStatus = GoalStatus;

	if( unit_navigator_debug.GetBool() )
	{
		if( LastGoalStatus != GoalStatus )
		{
			DevMsg( "#%d UnitNavigator: Goal status changed from %d to %d (goalflags %d)\n", 
				GetOuter()->entindex(), LastGoalStatus, GoalStatus, GetPath()->m_iGoalFlags );
		}
	}

#ifdef ENABLE_PYTHON
	boost::python::object curPath = m_refPath;
#endif // ENABLE_PYTHO
	
	// Dispatch events to AI based on our new goal status if needed
	if( GoalStatus == CHS_ATGOAL )
	{
		if( GetPath()->m_iGoalFlags & GF_NOCLEAR )
		{
			if( LastGoalStatus != CHS_ATGOAL )
			{
				// Notify AI we arrived at our goal
				DispatchOnNavAtGoal();
			}
		}
		else
		{
			// Notify AI we are at our goal and completed the path
			DispatchOnNavComplete();
		}
	}
	else if( GoalStatus == CHS_HASGOAL )
	{
		if( GetPath()->m_iGoalFlags & GF_NOCLEAR )
		{
			// Notify AI we lost our goal
			if( LastGoalStatus == CHS_ATGOAL )
			{
				DispatchOnNavLostGoal();
			}
		}
	}
	else if( GoalStatus == CHS_FAILED )
	{
		if( unit_navigator_debug.GetBool() )
			DevMsg("#%d UnitNavigator: Failed to reach goal. Dispatching failed.\n", GetOuter()->entindex());
		MoveCommand.Clear(); // Should we clear here? Or leave it to the event handler?
		DispatchOnNavFailed();
	}
	else if( GoalStatus == CHS_CLIMB )
	{
		// Dispatch climb event to AI, so it can start the climb code
		if( unit_navigator_debug.GetBool() )
			DevMsg("#%d UnitNavigator: Encounter climb obstacle. Dispatching OnStartClimb.\n", GetOuter()->entindex());
#ifdef ENABLE_PYTHON
		SrcPySystem()->Run<const char *, float, Vector>( 
			SrcPySystem()->Get("DispatchEvent", GetOuter()->GetPyInstance() ), 
			"OnStartClimb", m_fClimbHeight, m_vecClimbDirection
		);
#endif // ENABLE_PYTHON
	}
}

//-----------------------------------------------------------------------------
//  
//-----------------------------------------------------------------------------
void UnitBaseNavigator::UpdateFacingTargetState( bool bIsFacing )
{
	if( bIsFacing != m_bFacingFaceTarget )
	{
#ifdef ENABLE_PYTHON
		if( bIsFacing )
		{
			SrcPySystem()->Run<const char *>( 
				SrcPySystem()->Get("DispatchEvent", GetOuter()->GetPyInstance() ), 
				"OnFacingTarget"
			);
		}
		else
		{
			SrcPySystem()->Run<const char *>( 
				SrcPySystem()->Get("DispatchEvent", GetOuter()->GetPyInstance() ), 
				"OnLostFacingTarget"
			);
		}
#endif // ENABLE_PYTHON
		m_bFacingFaceTarget = bIsFacing;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates our preferred facing direction.
//			Defaults to the path direction.
//			TODO: Weapon shooting code depends on facing direction computed here.
//				  Make this more robust.
//-----------------------------------------------------------------------------
void UnitBaseNavigator::UpdateIdealAngles( UnitBaseMoveCommand &MoveCommand, Vector *pPathDir )
{
	bool bZeroPitch = GetOuter()->GetAnimState() ? !GetOuter()->GetAnimState()->HasAimPoseParameters() : false;

	Vector vFacingOrigin = m_pOuter->GetActiveWeapon() ? m_pOuter->Weapon_ShootPosition() : m_pOuter->EyePosition();

	// Update facing target if any
	// Call UpdateFacingTargetState after updating the idealangles, because it might clear
	// the facing target.
	if( m_fIdealYaw != -1 )	
	{
		MoveCommand.idealviewangles[PITCH] = MoveCommand.idealviewangles[ROLL] = 0.0f;
		MoveCommand.idealviewangles[YAW] = m_fIdealYaw;
		UpdateFacingTargetState( AngleDiff( m_fIdealYaw, GetAbsAngles()[YAW] ) <= m_fIdealYawTolerance );
	}
	else if( m_hFacingTarget ) 
	{
		Vector dir = m_hFacingTarget->BodyTarget( GetLocalOrigin() ) - vFacingOrigin;
		if( bZeroPitch )
			dir.z = 0;
		VectorAngles(dir, MoveCommand.idealviewangles);
		UpdateFacingTargetState( GetOuter()->FInAimCone( m_hFacingTarget, m_fFacingCone ) );
	}
	else if( m_vFacingTargetPos != vec3_origin )
	{
		Vector dir = m_vFacingTargetPos - vFacingOrigin;
		if( bZeroPitch )
			dir.z = 0;
		VectorAngles(dir, MoveCommand.idealviewangles);
		UpdateFacingTargetState( GetOuter()->FInAimCone(m_vFacingTargetPos, m_fFacingCone) );
	}
	// Face enemy for shooting by default
	// TODO: This should be controlled from the AI action
	else if( m_pOuter->GetEnemy() )
	{
		Vector dir = m_pOuter->GetEnemy()->BodyTarget( GetLocalOrigin() ) - vFacingOrigin;
		if( bZeroPitch )
			dir.z = 0;
		VectorAngles(dir, MoveCommand.idealviewangles);
	}
	// Face path dir if we are following a path
	else if( pPathDir ) 
	{
		VectorAngles(*pPathDir, MoveCommand.idealviewangles);
		if( bZeroPitch )
			MoveCommand.idealviewangles.x = 0;
	}
}

//-----------------------------------------------------------------------------
// Calculate the move parameters for the given angles  
//-----------------------------------------------------------------------------
void UnitBaseNavigator::CalcMove( UnitBaseMoveCommand &MoveCommand, QAngle angles, float speed )
{
	if( unit_navigator_eattest.GetBool() )
		return;

	float fYaw, angle, fRadians;
	Vector2D move;

	fYaw = anglemod(angles[YAW]);

	angle = anglemod( MoveCommand.viewangles.y - fYaw );

	fRadians = angle / 57.29578f;	// To radians

	move.x = cos(fRadians);
	move.y = sin(fRadians);

	Vector2DNormalize( move );

	MoveCommand.forwardmove = move.x * speed;
	MoveCommand.sidemove = move.y * speed;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float UnitBaseNavigator::GetDensityMultiplier()
{
	if( GetOuter()->GetCommander() )
		return 10.0f; // Move aside!
	if( GetPath()->m_iGoalType != GOALTYPE_NONE )
		return 1.1f;
	return 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnitBaseNavigator::RegenerateConsiderList( UnitBaseMoveCommand &MoveCommand, Vector &vPathDir, CheckGoalStatus_t GoalStatus  )
{
	VPROF_BUDGET( "UnitBaseNavigator::RegenerateConsiderList", VPROF_BUDGETGROUP_UNITS );

	CollectConsiderEntities( MoveCommand, GoalStatus );
	ComputeConsiderDensAndDirs( MoveCommand, vPathDir, GoalStatus );
}

//-----------------------------------------------------------------------------
// Purpose: Generates a list of surrounding entities, potentially blocking
//			the unit.
//-----------------------------------------------------------------------------
void UnitBaseNavigator::CollectConsiderEntities( UnitBaseMoveCommand &MoveCommand, CheckGoalStatus_t GoalStatus )
{
	int n, i;
	float fRadius;
	CBaseEntity *pEnt;
	CBaseEntity *pList[CONSIDER_SIZE];

	const Vector &origin = GetAbsOrigin();
	fRadius = GetEntityBoundingRadius( m_pOuter );

	// Detect nearby entities
	m_iConsiderSize = 0;
	
	float fBoxHalf = fRadius * unit_consider_multiplier.GetFloat();
	//Ray_t ray;
	//ray.Init( origin, origin+Vector(0,0,1), -Vector(fBoxHalf, fBoxHalf, 0.0f), Vector(fBoxHalf, fBoxHalf, fBoxHalf) );
	//n = UTIL_EntitiesAlongRay( pList, CONSIDER_SIZE, ray, 0 );
	n = UTIL_EntitiesInSphere( pList, CONSIDER_SIZE, origin, fBoxHalf, 0 );

	m_vBlockingDirection = vec3_origin;
	int iNumBlockers = 0;

	// Generate list of entities we want to consider
	for( i = 0; i < n; i++ )
	{
		pEnt = pList[i];

		// Test if we should consider this entity
		if( pEnt == m_pOuter || pEnt->DensityMap()->GetType() == DENSITY_NONE || !pEnt->IsSolid() || 
			pEnt->IsNavIgnored() || pEnt->GetOwnerEntity() == m_pOuter || (pEnt->GetFlags() & (FL_STATICPROP|FL_WORLDBRUSH)) )
			continue;

		// RegenerateConsiderList only runs with CHS_ATGOAL with the GF_ATGOAL_RELAX flag
		// In this case, we want to avoid the target, so it should not be filtered (used for following code)
		if( GoalStatus != CHS_ATGOAL )
		{
			if( pEnt == GetPath()->m_hTarget )
				continue;

			// MAYBE?
#if 0
			if( GetPath()->m_iGoalFlags & GF_OWNERISTARGET )
			{
				CBaseEntity *pOwnerEntity = pEnt->GetOwnerEntity();
				if( pOwnerEntity && pOwnerEntity == GetPath()->m_hTarget.Get() )
					continue;
			}
#endif // 0
		}

		// If specified, don't avoid enemies
		if( !GetPath()->m_bAvoidEnemies )
		{
			if( m_pOuter->IRelationType( pEnt ) == D_HT )
				continue;
		}

		// Ignore units of who this unit is the follow target
		// Otherwise we might move out of the way!
		if( pEnt->MyUnitPointer() )
		{
			UnitBaseNavigator *pNavigator = pEnt->MyUnitPointer()->GetNavigator();
			if( pNavigator )
			{
				if( pNavigator->GetPath()->m_iGoalType == GOALTYPE_TARGETENT && pNavigator->GetPath()->m_hTarget == m_pOuter )
					continue;
			}
		}

		// In list of ignored nav entities last locomotion run
		if( IsEntityNavIgnored( MoveCommand, pEnt ) )
			continue;

		// Store general info
		m_ConsiderList[m_iConsiderSize] = pEnt;
		m_iConsiderSize++;

		if( MoveCommand.HasBlocker( pEnt ) )
		{
			Vector vBlockDir = pEnt->GetAbsOrigin() - GetAbsOrigin();
			vBlockDir.z = 0;
			m_vBlockingDirection += (1/vBlockDir.Length2D()) * vBlockDir;
			iNumBlockers += 1;
		}
	}

	if( iNumBlockers > 0 )
	{
		m_vBlockingDirection /= iNumBlockers;
		VectorNormalize( m_vBlockingDirection );
		m_vBlockingDirection = m_vBlockingDirection.Cross( Vector(0, 0, 1) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool UnitBaseNavigator::IsEntityNavIgnored( UnitBaseMoveCommand &MoveCommand, CBaseEntity *pEnt )
{
	for( int i = 0; i < MoveCommand.navignorelist.Count(); i++ )
	{
		if( MoveCommand.navignorelist[i] && MoveCommand.navignorelist[i] == pEnt )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Calculates the allowed move directions and densities in those directions
//-----------------------------------------------------------------------------
void UnitBaseNavigator::ComputeConsiderDensAndDirs( UnitBaseMoveCommand &MoveCommand, Vector &vPathDir, CheckGoalStatus_t GoalStatus )
{
	int j;
	const Vector &origin = GetAbsOrigin();
	float fRadius = GetEntityBoundingRadius(m_pOuter);

	// Reset list information
	m_iUsedTestDirections = 0;

	// Try testing a bit further in case we are stuck
	if( GetBlockedStatus() >= BS_MUCH )
		fRadius *= 3.0f;
	else if( GetBlockedStatus() >= BS_LITTLE )
		fRadius *= 2.0f;

	// Compute densities
	if( GoalStatus == CHS_NOGOAL || GoalStatus == CHS_ATGOAL )
	{
		m_pOuter->GetVectors(&m_vTestDirections[m_iUsedTestDirections], NULL, NULL); // Just use forward as start dir
		m_vTestPositions[m_iUsedTestDirections] = origin + m_vTestDirections[m_iUsedTestDirections] * fRadius;
		ComputeDensityAndAvgVelocity( m_iUsedTestDirections, MoveCommand );

		m_iUsedTestDirections += 1;

		// Full circle scan
		for( j = 0; j < 7; j++ )
		{
			VectorYawRotate(m_vTestDirections[m_iUsedTestDirections-1], 45.0f, m_vTestDirections[m_iUsedTestDirections]);
			m_vTestPositions[m_iUsedTestDirections] = origin + m_vTestDirections[m_iUsedTestDirections] * fRadius;
			ComputeDensityAndAvgVelocity( m_iUsedTestDirections, MoveCommand );

			m_iUsedTestDirections += 1;
		}
	}
	else
	{
		// Compute density path direction
		m_vTestDirections[m_iUsedTestDirections] = vPathDir;
		m_vTestPositions[m_iUsedTestDirections] = origin + m_vTestDirections[m_iUsedTestDirections] * fRadius;
		float fTotalDensity = ComputeDensityAndAvgVelocity( m_iUsedTestDirections, MoveCommand );

		// Half circle scan with mid at waypoint direction (except if stuck, then do a full scan)
		// Scan starts in the middle, alternating between the two different directions
		// Scan breaks early if:
		//		- fTotalDensity is very low for the scanned direction.
		//		- No seeds
		//		- Not blocked
		float fRotate = 45.0f;
		j = 0;
		int jmax = GetBlockedStatus() >= BS_LITTLE ? 7 : 4; // Do full scan in case we are stuck
		if( m_bLimitPositionActive )
			jmax = 1;

		while( j < jmax && ( GetBlockedStatus() != BS_NONE || m_Seeds.Count() || fTotalDensity > 0.01f ) )
		{
			m_iUsedTestDirections += 1;
			j++;

			VectorYawRotate(m_vTestDirections[m_iUsedTestDirections-1], fRotate, m_vTestDirections[m_iUsedTestDirections]);
			m_vTestPositions[m_iUsedTestDirections] = origin + m_vTestDirections[m_iUsedTestDirections] * fRadius;
			fTotalDensity = ComputeDensityAndAvgVelocity( m_iUsedTestDirections, MoveCommand );

			fRotate *= -1;
			if( fRotate < 0.0 )
				fRotate -= 45.0f;
			else
				fRotate += 45.0f;
		}

		m_iUsedTestDirections += 1;
	}
}

#if 0
//-----------------------------------------------------------------------------
// Purpose: Determine whether the entity can affect our flow our not.
//-----------------------------------------------------------------------------
bool UnitBaseNavigator::ShouldConsiderEntity( CBaseEntity *pEnt )
{
	// Shouldn't consider ourself or anything that is not solid.
	if( pEnt == m_pOuter || !pEnt->IsSolid() )
		return false;

	// Skip target of our path, otherwise we might not be able to get near.
	if( pEnt == GetPath()->m_hTarget )
		return false;

	return true;
}
#endif // 0

//-----------------------------------------------------------------------------
// Purpose: Whether we should add density from nav areas
//-----------------------------------------------------------------------------
bool UnitBaseNavigator::ShouldConsiderNavMesh( void )
{
	if( unit_cost_nonavareacheck.GetBool() )
		return false;
	// m_bNoNavAreasNearby is set when in BS_STUCK and we can't find anything nearby 
	// Also don't consider nav meshes when really nearby our goal or when this is a direct path
	if( GetPath()->m_bIsDirectPath || m_bNoNavAreasNearby || m_fGoalDistance < 32.0f ) 
		return false;
	UnitBaseWaypoint *pCurWaypoint = GetPath()->m_pWaypointHead;
	return TheNavMesh->IsLoaded() && ( !pCurWaypoint || pCurWaypoint->SpecialGoalStatus == CHS_NOGOAL );
}

//-----------------------------------------------------------------------------
// Purpose: Computes the density and average velocity for a given direction.
//-----------------------------------------------------------------------------
float UnitBaseNavigator::ComputeDensityAndAvgVelocity( int iPos, UnitBaseMoveCommand &MoveCommand )
{
	VPROF_BUDGET( "UnitBaseNavigator::ComputeDensityAndAvgVelocity", VPROF_BUDGETGROUP_UNITS );

	Vector &vAvgVelocity = m_vAverageVelocities[iPos];

	CBaseEntity *pEnt;
	int i;
	float fSumDensity;
	fSumDensity = 0.0f;
	
	vAvgVelocity.Init( 0, 0, 0 );

	const Vector &vPos = m_vTestPositions[ iPos ];

	// Add in all entities we are considering
	for( i = 0; i < m_iConsiderSize; i++ )
	{
		pEnt = m_ConsiderList[i];
		if( !pEnt )
			continue;

		float fEntDensity = GetEntityDensity( vPos, m_ConsiderList[i] );
		fSumDensity += fEntDensity;

		if( pEnt->GetMoveType() == MOVETYPE_NONE || pEnt->GetAbsVelocity().Length2D() < 25.0f )
		{
			// Non-moving units should generate an outward velocity
			Vector vDir = vPos - pEnt->GetAbsOrigin();
			vDir.z = 0.0f;
			float fSpeed = VectorNormalize(vDir);
			fSpeed = fEntDensity * 2000.0f;
			vAvgVelocity += fEntDensity * vDir * fSpeed;
		}
		else
		{
			// Moving units generate a flow velocity
			vAvgVelocity += fEntDensity * pEnt->GetAbsVelocity();
		}
	}	

	// Consider navigation mesh
	if( m_fIgnoreNavMeshTime < gpGlobals->curtime )
	{
		CBaseEntity *pGroundEntity = m_pOuter->GetGroundEntity();

		if( pGroundEntity && pGroundEntity->IsBaseTrain() )
		{
			// TODO: Generate density at edges while moving
			//		 For now, assume mapper blocks the edges
		}
		else
		{
#if 0
			// Add density from nav mesh (don't drop off cliffs when we don't want too)
			// Note that we don't do this for special waypoints (climbing/dropping/etc)
			if( ShouldConsiderNavMesh() )
			{
				UnitShortestPathCost costFunc(m_pOuter);

				// Add density from nav area
				// TODO: Check if target area is valid? 
				CNavArea *pAreaTo = TheNavMesh->GetNavArea( vPos + Vector(0,0,96.0f), 150.0f );
				if( !pAreaTo /*|| !costFunc.IsAreaValid( pAreaTo )*/ )
				{
					float fDensity = THRESHOLD_MAX / 2.0f;
					fSumDensity += fDensity;
					Vector vDir = GetAbsOrigin() - vPos;
					VectorNormalize(vDir);
					vAvgVelocity += fDensity * vDir * MoveCommand.maxspeed;
				}
			}
#endif // 0

			if( m_Seeds.Count() )
			{
				float fDist;
				float fRadius = GetEntityBoundingRadius(m_pOuter) * unit_seed_radius_bloat.GetFloat();
				float fBaseDensitySeed = unit_seed_density.GetFloat();

				// Increase seed density + radius a bit when blocked for a longer time
				if( GetBlockedStatus() >= BS_MUCH )
				{
					fBaseDensitySeed *= 1.5f;
					fRadius *= 1.2f;
				}

				// Add seeds if in range
				for( int i = 0; i < m_Seeds.Count(); i++ )
				{
					fDist = (vPos.AsVector2D() - m_Seeds[i].m_vPos).Length();
					if( fDist < fRadius )
						fSumDensity += fBaseDensitySeed - ( (fDist / fRadius) * fBaseDensitySeed );
				}
			}
		}
	}

	// If our position is limited, make the density very high outside our limited radius
	if( m_bLimitPositionActive )
	{
		float fDistSqr2D = m_vLimitPositionCenter.AsVector2D().DistToSqr( vPos.AsVector2D() );
		float fDistSqrMax = m_fLimitPositionRadius * m_fLimitPositionRadius;
		//if( fDistSqr2D > fDistSqrMax )
		{
			float fDensity = THRESHOLD_MAX * (fDistSqr2D / fDistSqrMax) / 2.0f;
			//engine->Con_NPrintf( iPos, "iPos: %d, Density: %f, dist: %f", iPos, fDensity, m_vLimitPositionCenter.AsVector2D().DistTo( m_vTestPositions[iPos].AsVector2D() ));
			fSumDensity += fDensity;
			Vector vDir = m_vLimitPositionCenter - GetAbsOrigin();
			VectorNormalize(vDir);
			vAvgVelocity += fDensity * vDir * MoveCommand.maxspeed;
		}
	}

	// Average the velocity
	// FIXME: Find out why the avg is sometimes invalid
	if( fSumDensity < FLT_EPSILON || !vAvgVelocity.IsValid() )
		vAvgVelocity = vec3_origin;
	else
		vAvgVelocity /= fSumDensity;

	m_DensitySums[ iPos ] = fSumDensity;

	return fSumDensity;

}

//-----------------------------------------------------------------------------
// Purpose: Computes the cost for going in a test direction.
//-----------------------------------------------------------------------------
float UnitBaseNavigator::ComputeUnitCost( int iPos, Vector *pFinalVelocity, CheckGoalStatus_t GoalStatus, 
										 UnitBaseMoveCommand &MoveCommand, Vector &vGoalPathDir, const float &fWaypointDist, float &fDensity )
{
	VPROF_BUDGET( "UnitBaseNavigator::ComputeUnitCost", VPROF_BUDGETGROUP_UNITS );

	float fSpeed, fDist;
	Vector vFlowDir, vPathVelocity, vFlowVelocity, vFinalVelocity, vPathDir;

	// Dist to next waypoints + speed predicted position
	if( GetPath()->m_iGoalType != GOALTYPE_NONE && GoalStatus != CHS_ATGOAL )
		fDist = UnitComputePathDirection2(m_vTestPositions[iPos], GetPath()->m_pWaypointHead, vPathDir);
	else if( m_vForceGoalVelocity.IsValid() )
		fDist = ( (m_vForceGoalVelocity * 2) - m_vTestPositions[iPos] ).Length2D();
	else
		fDist = 0.0f;
	
	fDensity = m_DensitySums[ iPos ];
	const Vector &vAvgVelocity = m_vAverageVelocities[ iPos ];

	// Compute path speed
	if( !m_bNoPathVelocity && GoalStatus != CHS_NOGOAL && GoalStatus != CHS_ATGOAL ) {
		// Desire full speed when not near the final goal
		float fMaxTravelDist = MoveCommand.maxspeed * MoveCommand.interval;
		if( (fWaypointDist-MoveCommand.stopdistance) <= fMaxTravelDist )
			vPathVelocity = ((fWaypointDist-MoveCommand.stopdistance)/MoveCommand.interval) * m_vTestDirections[iPos];
		else
			vPathVelocity = MoveCommand.maxspeed * m_vTestDirections[iPos];
	}
	else
	{	
		vPathVelocity = vec3_origin;
	}

	// Compute flow speed
	vFlowVelocity = vAvgVelocity;

	// Zero out flow velocity if too low. Otherwise it results in retarded movement.
	if( vFlowVelocity.Length2D() < 15.0f )
		vFlowVelocity.Zero();
	
	// Depending on the thresholds use path, flow or interpolated speed
	float fThresholdMin = THRESHOLD_MIN;
	float fThresholdMax = THRESHOLD_MAX;
	if( fDensity < fThresholdMin )
	{
		vFinalVelocity = vPathVelocity;
	}
	else if (fDensity > fThresholdMax )
	{
		vFinalVelocity = vFlowVelocity;
	}
	else
	{
		// Computed interpolated speed
		vFinalVelocity = vPathVelocity + ( ( fDensity - fThresholdMin ) / ( fThresholdMax - fThresholdMin ) ) * ( vFlowVelocity - vPathVelocity );
	}

	fSpeed = vFinalVelocity.Length2D();
	*pFinalVelocity = vFinalVelocity;

	// Velocity when having no goal is solely based on the density (i.e. just move away if something is getting close)
	if( GoalStatus == CHS_NOGOAL || GoalStatus == CHS_ATGOAL || fSpeed < FLT_EPSILON ) 
	{
		return fDensity; 
	}

	// Return weighted cost based on the following parameters:
	// 1. The new distance to the next waypoint when going in this direction
	// 2. The disconform the unit feels due the density in the tested direction
	// 3. The "free" non blocked directions
	float cost = ( unit_cost_distweight.GetFloat() * fDist ) + 
			 ( m_fDiscomfortWeight * fDensity );
	if( m_vBlockingDirection != vec3_origin )
	{
		if( m_fNextChooseDirection < gpGlobals->curtime )
		{
			//float fDist1 = (vFinalVelocity.Normalized() - m_vBlockingDirection).Length2D();
			//float fDist2  = (vFinalVelocity.Normalized() + m_vBlockingDirection).Length2D();
			m_iBlockResolveDirection = random->RandomInt(0, 1) > 0 ? 1 : -1;
			m_fNextChooseDirection = gpGlobals->curtime + 5.0f;
		}

		cost += (vFinalVelocity.Normalized() - (m_vBlockingDirection * m_iBlockResolveDirection)).Length2D() * unit_cost_blockdirweight.GetFloat();
	}
	return cost;
}

//-----------------------------------------------------------------------------
// Purpose: Computes our velocity by looking at the densities around us.
//			The unit will try to move in the path direction, but prefers
//			to go into a low density direction.
//-----------------------------------------------------------------------------
Vector UnitBaseNavigator::ComputeVelocity( CheckGoalStatus_t GoalStatus, UnitBaseMoveCommand &MoveCommand, Vector &vPathDir, const float &fWaypointDist )
{
	VPROF_BUDGET( "UnitBaseNavigator::ComputeVelocity", VPROF_BUDGETGROUP_UNITS );

#if 0
	// Don't go out of "ignore nav areas" mode until we found a valid nav area.
	if( m_bNoNavAreasNearby )
	{
		UnitShortestPathCost costFunc(m_pOuter);
		CNavArea *pMyArea = TheNavMesh->GetNavArea(m_pOuter->EyePosition(), 150.0f);
		if( pMyArea && costFunc.IsAreaValid( pMyArea ) )
		{
			m_bNoNavAreasNearby = false;
		}
	}
#endif // 0
	// By default we are moving into the direction of the next waypoint
	// We now calculate the velocity based on current velocity, flow, density, etc
	Vector vBestVel, vVelocity;
	float fCost, fBestCost, fComputedDensity;
	int i;

	fBestCost = 999999999.0f;
	vBestVel.Zero();

	if( GoalStatus == CHS_NOGOAL || GoalStatus == CHS_ATGOAL )
	{
		// Don't have a goal, in this case we don't look at the flow speed
		// Instead we try to find a position in which highest and lowest density surrounding us doesn't 
		// differ too much.
		float fHighestDensity = 0.0f;
		int pos = -1;
		for( i = 0; i < m_iUsedTestDirections; i++ )	
		{
			fCost = ComputeUnitCost( i, &vVelocity, GoalStatus, MoveCommand, vPathDir, fWaypointDist, fComputedDensity );
			if( fComputedDensity > fHighestDensity )
				fHighestDensity = fComputedDensity;
			if( fCost < fBestCost )
			{
				fBestCost = fCost;
				m_fLastBestDensity = fComputedDensity;
				m_iDebugBestDir = i;
				pos = i;
			}
		}

		// Move away if in some direction the density becomes high and if there is a much better spot
		if( fHighestDensity - m_fLastBestDensity > unit_nogoal_mindiff.GetFloat() && 
			fHighestDensity > unit_nogoal_mindest.GetFloat() )
		{
			vBestVel = m_vTestDirections[pos] * (fHighestDensity-m_fLastBestDensity) * MoveCommand.maxspeed;
		}
	}
	else
	{
		// Find best cost and use that speed + direction
		for( i = 0; i < m_iUsedTestDirections; i++ )	
		{
			fCost = ComputeUnitCost( i, &vVelocity, GoalStatus, MoveCommand, vPathDir, fWaypointDist, fComputedDensity );
			if( fCost < fBestCost )
			{
				fBestCost = fCost;
				m_fLastBestDensity = fComputedDensity;
				m_iDebugBestDir = i;
				vBestVel = vVelocity;
			}
		}

		if( GetBlockedStatus() >= BS_STUCK && m_fLastBestDensity > 90.0f )
		{
			// NOTE: Stuck because there are no nav areas around (density higher than 90). Just try moving!
			vBestVel = vPathDir * MoveCommand.maxspeed; 
			m_bNoNavAreasNearby = true;
		}

		// Update discomfort weight
		if( m_fLastBestDensity > unit_cost_discomfortweight_growthreshold.GetFloat() )
		{
			m_fDiscomfortWeight = Min( m_fDiscomfortWeight+MoveCommand.interval * unit_cost_discomfortweight_growrate.GetFloat(),
										unit_cost_discomfortweight_max.GetFloat() );
		}
		else
		{
			m_fDiscomfortWeight = Max( m_fDiscomfortWeight-MoveCommand.interval * unit_cost_discomfortweight_growrate.GetFloat(),
										unit_cost_discomfortweight_start.GetFloat() );
		}
	}

	if( GetOuter()->m_debugOverlays != 0 )
	{
		m_vDebugVelocity = vBestVel;
		m_fDebugLastBestCost = fBestCost;
		m_DebugLastMoveCommand = MoveCommand;
	}

	return vBestVel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float UnitBaseNavigator::CalculateAvgDistHistory()
{
	int i;
	float fAvgDist;

	if( m_DistHistory.Count() == 0 )
		return -1;

	fAvgDist = 0.0f;
	for( i=m_DistHistory.Count()-1; i >= 0; i-- )
		fAvgDist += m_DistHistory[i].m_fDist;
	fAvgDist /= m_DistHistory.Count();

	m_DistHistory.RemoveAll();
	return fAvgDist;
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnitBaseNavigator::InsertSeed( const Vector &vPos )
{
	m_Seeds.AddToTail( seed_entry_t( vPos.AsVector2D(), gpGlobals->curtime ) );
	if( unit_navigator_debug.GetBool() )
	{
		if( unit_navigator_debug.GetInt() > 1 )
			NDebugOverlay::Box( vPos, -Vector(2, 2, 2), Vector(2, 2, 2), 0, 255, 0, 255, 1.0f );
		NavDbgMsg("#%d: InsertSeed: Added density seed (total seeds: %d)\n", GetOuter()->entindex(), m_Seeds.Count() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Do goal checks and update our path
//			Returns the new goal status.
//-----------------------------------------------------------------------------
CheckGoalStatus_t UnitBaseNavigator::UpdateGoalAndPath( UnitBaseMoveCommand &MoveCommand, Vector &vPathDir, float &fWaypointDist )
{
	// Reset here, because it might not regenerate the list
	m_iConsiderSize = 0;
	m_iUsedTestDirections = 0;

	// Reset current goal dependency
	m_hAtGoalDependencyEnt = NULL;

	// In case we have an target ent (note: before UpdateGoalInfo, because you can't update goal info if target is lost)
	if( GetPath()->m_iGoalType == GOALTYPE_TARGETENT || GetPath()->m_iGoalType == GOALTYPE_TARGETENT_INRANGE )
	{
		// Check if still exists and alive.
		if( !GetPath()->m_hTarget )
		{
			if( unit_navigator_debug.GetBool() )
				DevMsg("#%d UnitNavigator: Target lost\n", GetOuter()->entindex());
			return CHS_FAILED;
		}

		if( (GetPath()->m_iGoalFlags & GF_REQTARGETALIVE) && !GetPath()->m_hTarget->IsAlive() )
		{
			if( unit_navigator_debug.GetBool() )
				DevMsg("#%d UnitNavigator: Target not alive\n", GetOuter()->entindex());
			return CHS_FAILED;
		}

		// Check if the target ent moved into another area. In that case recalculate the path.
		// In the other case just update the goal pos (quick).
		// Note that after a path recomputation, we won't try to recompute the path for a while anymore (performance)
		//const Vector &vTargetOrigin = GetPath()->m_hTarget->EyePosition(); // Eye position may be too high for some targets
		const Vector &vTargetOrigin = GetPath()->m_hTarget->WorldSpaceCenter();

		if( (gpGlobals->curtime - m_fLastPathRecomputation) > 8.0f )
		{
			CNavArea *pTargetArea = TheNavMesh->GetNearestNavArea( vTargetOrigin, true, 256.0f, false, false );
			CNavArea *pGoalArea = TheNavMesh->GetNavAreaByID( m_iLastTargetArea );
			if( !pGoalArea )
				pGoalArea = TheNavMesh->GetNearestNavArea( GetPath()->m_vGoalPos, true, 256.0f, false, false );

			if( pTargetArea && pGoalArea && pTargetArea != pGoalArea )
			{
				if( unit_navigator_debug.GetBool() )
					DevMsg("#%d UnitNavigator: Target changed area (%d -> %d). Recomputing path...\n", 
						GetOuter()->entindex(), pGoalArea->GetID(), pTargetArea->GetID() );

				// Update goal target position and recompute
				GetPath()->m_vGoalPos =vTargetOrigin;
				if( GetPath()->m_iGoalType == GOALTYPE_TARGETENT_INRANGE )
					DoFindPathToPosInRange();
				else
					DoFindPathToPos();

				// Store area id of the goal for the next time we check
				m_iLastTargetArea = pGoalArea->GetID();
			}
			else
			{
				// Just update goal target position and update the last waypoint
				GetPath()->m_vGoalPos = vTargetOrigin;
				GetPath()->m_pWaypointHead->GetLast()->SetPos(GetPath()->m_vGoalPos);
			}
		}
		else
		{
			// Just update goal target position and update the last waypoint
			GetPath()->m_vGoalPos = vTargetOrigin;
			GetPath()->m_pWaypointHead->GetLast()->SetPos(GetPath()->m_vGoalPos);
		}
	}

	// Store distance and range
	UpdateGoalInfo();

	// Check distance we moved. Might have a max distance we may move to a target
	if( GetPath()->m_iGoalType != GOALTYPE_NONE && GetPath()->m_fMaxMoveDist != 0 )
	{
		if( GetPath()->m_fMaxMoveDist < (GetPath()->m_vStartPosition - GetAbsOrigin()).Length2D() )
		{
			if( unit_navigator_debug.GetBool() )
				DevMsg("#%d UnitNavigator: max move distance exceeded\n", GetOuter()->entindex());
			return CHS_FAILED;
		}
	}

	// Check if we bumped into our target goal. In that case we are done.
	if( GetPath()->m_iGoalType == GOALTYPE_TARGETENT )
	{
		// If we are currently at our goal, we will simplify the check if GF_ATGOAL_RELAX is active
		// We just need to be "near" the goal. m_fAtGoalTolerance is set at the time we first arrived at the goal.
		if( m_LastGoalStatus == CHS_ATGOAL && ( GetPath()->m_iGoalFlags & GF_ATGOAL_RELAX ) )
		{
			if( m_fGoalDistance < GetPath()->m_fAtGoalTolerance )
			{
				RegenerateConsiderList( MoveCommand, vPathDir, CHS_ATGOAL );
				return CHS_ATGOAL;
			}
		}
		else
		{
			// Goal is satisfied if our blocker is the target or if we are REALLY close
			if( MoveCommand.HasBlocker( GetPath()->m_hTarget ) || m_fGoalDistance < GetEntityBoundingRadius(m_pOuter)*2.0f || TestNearbyUnitsWithSameGoal( MoveCommand ) )
			{
				return CHS_ATGOAL;
			}
			else if( (GetPath()->m_iGoalFlags & GF_OWNERISTARGET) && MoveCommand.HasBlockerWithOwnerEntity( GetPath()->m_hTarget ) )
			{
				return CHS_ATGOAL;
			}
		}
	}
	// Check for any of the range goal types if we are in range. In that we are done.
	else if( GetPath()->m_iGoalType == GOALTYPE_TARGETENT_INRANGE || GetPath()->m_iGoalType == GOALTYPE_POSITION_INRANGE )
	{
		if( IsInRangeGoal( MoveCommand ) )
		{
			return CHS_ATGOAL;
		}
	}

	// Made it to here, so update our path if any
	bool bPathBlocked = false;
	CheckGoalStatus_t GoalStatus = CHS_NOGOAL;
	vPathDir = vec3_origin;
	if( GetPath()->m_iGoalType != GOALTYPE_NONE )
	{
		// Advance path
		GoalStatus = MoveUpdateWaypoint( MoveCommand );
		if( GoalStatus != CHS_ATGOAL )
		{
			if( unit_reactivepath.GetBool() && m_fNextReactivePathUpdate < gpGlobals->curtime ) {
				m_fNextReactivePathUpdate = gpGlobals->curtime + 0.25f;
				bPathBlocked = UpdateReactivePath();
			}

			fWaypointDist = ComputeWaypointDistanceAndDir( vPathDir );
		}

		// Path might be blocked. Recompute or add density seeds
		if( bPathBlocked || GetBlockedStatus() > BS_NONE )
		{
			if( m_fNextAllowPathRecomputeTime < gpGlobals->curtime )
			{
				if( unit_navigator_debug.GetBool() )
				{
					DevMsg("#%d UnitNavigator: path blocked, recomputing path...(reasons: ", GetOuter()->entindex());
					if( bPathBlocked )
						DevMsg("blocked path detected");
					if( GetBlockedStatus() > BS_NONE )
						DevMsg(", blocked status higher than BS_NONE");
					for( int i = 0; i < MoveCommand.blockers.Count(); i++ )
					{
						CBaseEntity *pBlocker = MoveCommand.blockers[i].blocker;
						if( !pBlocker )
							continue;

						DevMsg(", blocked entity: #%d - %s", pBlocker->entindex(), pBlocker->GetClassname());
					}
					DevMsg(")\n");
				}

				// Recompute path
				if( GetPath()->m_iGoalType == GOALTYPE_TARGETENT_INRANGE || GetPath()->m_iGoalType == GOALTYPE_POSITION_INRANGE )
					DoFindPathToPosInRange();
				else
					DoFindPathToPos();
				fWaypointDist = ComputeWaypointDistanceAndDir( vPathDir );
				m_iBlockedPathRecomputations++;

				// Don't allow too many recomputations
				if( GetBlockedStatus() >= BS_STUCK )
					m_fNextAllowPathRecomputeTime = gpGlobals->curtime + random->RandomFloat(7.0f, 12.0f);
				else if( GetBlockedStatus() >= BS_MUCH )
					m_fNextAllowPathRecomputeTime = gpGlobals->curtime + random->RandomFloat(4.0f, 5.0f);
				else if( GetBlockedStatus() >= BS_LITTLE )
					m_fNextAllowPathRecomputeTime = gpGlobals->curtime + random->RandomFloat(2.0f, 3.0f);
				else
					m_fNextAllowPathRecomputeTime = gpGlobals->curtime + random->RandomFloat(1.0f, 2.0f);
			}

			if( GetBlockedStatus() >= BS_LITTLE )
			{
				// Apparently we are stuck, so try to add a seed that serves as density point
				for( int i = 0; i < MoveCommand.blockers.Count(); i++ )
				{
					if( !MoveCommand.blockers[i].blocker )
						continue;

					Vector vHitPos = MoveCommand.blockers[i].blocker_hitpos + MoveCommand.blockers[i].blocker_dir * m_pOuter->CollisionProp()->BoundingRadius2D();
					InsertSeed( vHitPos );
				}
			}
		}
	}

	// Generate a list of entities surrounding us
	// Do this right after updating our path, so we use the correct waypoints
	RegenerateConsiderList( MoveCommand, vPathDir, GoalStatus );

	if( GoalStatus != CHS_NOGOAL )
	{
		if( m_Seeds.Count() )
		{
			// Clear old entries
			int i;
			for( i = m_Seeds.Count()-1; i >= 0; i-- )
			{
				if( m_Seeds[i].m_fTimeStamp + unit_seed_historytime.GetFloat() < gpGlobals->curtime )
				{
					m_Seeds.Remove(i);
				}
			}
		}

#if 0
		if( m_fNextAvgDistConsideration < gpGlobals->curtime )
		{
			float fAvgDist = CalculateAvgDistHistory();
			if( fAvgDist != -1 )
			{
				if( GetPath()->CurWaypointIsGoal() && m_bUnitArrivedAtSameGoalNearby )
				{
					if( m_fLastAvgDist != -1 && fAvgDist > (m_fLastAvgDist + unit_cost_minavg_improvement.GetFloat()) )
					{
						GoalStatus = CHS_ATGOAL;
						if( unit_navigator_debug.GetBool() )
							DevMsg("#%d: UpdateGoalAndPath: Arrived at goal due no improvement and nearby units arrived with same goal\n", GetOuter()->entindex() );
					}
				}
				m_fNextAvgDistConsideration = gpGlobals->curtime + unit_cost_history.GetFloat();
				m_fLastAvgDist = fAvgDist;
			}
		}
		m_DistHistory.AddToTail( dist_entry_t( (m_vLastPosition - GetAbsOrigin()).Length2D(), gpGlobals->curtime ) );
		
		if( m_fLastAvgDist != - 1 && m_fLastAvgDist < (MoveCommand.maxspeed * unit_cost_history.GetFloat()) / 2.0 )
		{
			m_Seeds.AddToTail( seed_entry_t( GetAbsOrigin().AsVector2D(), gpGlobals->curtime ) );
			if( unit_navigator_debug.GetBool() )
				DevMsg("#%d: UpdateGoalAndPath: Added seed due lack of improvement\n", GetOuter()->entindex() );
		}
#endif // 0
	}


	return GoalStatus;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool UnitBaseNavigator::TestNearbyUnitsWithSameGoal( UnitBaseMoveCommand &MoveCommand )
{
	if( unit_navigator_notestnearbyunits.GetBool() )
		return false;

	// Only when at least slightly blocked, except when we are at our goal
	// In that case we need to keep testing.
	if( m_LastGoalStatus != CHS_ATGOAL && GetBlockedStatus() < BS_LITTLE )
		return false;

	// The current path must be the goal waypoint
	if( !GetPath()->CurWaypointIsGoal() )
		return false;

	// Must be within goal tolerance
	if( GetGoalDistance() > GetPath()->m_fGoalTolerance )
		return false;

	for( int i = 0; i < MoveCommand.blockers.Count(); i++ )
	{
		CBaseEntity *pBlocker = MoveCommand.blockers[i].blocker;
		if( !pBlocker )
			continue;

		CUnitBase *pUnit = pBlocker->MyUnitPointer();
		if( !pUnit )
			continue;

		UnitBaseNavigator *pNavigator = pUnit->GetNavigator();
		if( !pNavigator )
			continue;

		while( pNavigator->GetAtGoalDependencyEnt() )
		{
			pNavigator = pNavigator->GetAtGoalDependencyEnt()->GetNavigator();
		}

		if( pNavigator == this )
			continue;

		if( pNavigator->GetGoalStatus() == CHS_ATGOAL && GetPath()->HasSamegoal( pNavigator->GetPath() ) )
		{
			m_hAtGoalDependencyEnt = pUnit;
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Checks if in range of our goal.
//-----------------------------------------------------------------------------
bool UnitBaseNavigator::IsInRangeGoal( UnitBaseMoveCommand &MoveCommand )
{
	if( GetPath()->m_hTarget )
	{
		// Check range
		if( GetPath()->m_fMaxRange == 0 )
		{
			// skip dist check if we have no minimum range and are bumping into the target or the dist is really close (might have the wrong blocker set), then we are in range
			float fTargetTolerance = GetEntityBoundingRadius(m_pOuter)*2.0f;
			bool bTouchingTarget = MoveCommand.HasBlocker( GetPath()->m_hTarget ) || MoveCommand.HasBlockerWithOwnerEntity( GetPath()->m_hTarget );
			if( !bTouchingTarget && m_fGoalDistance > fTargetTolerance && !TestNearbyUnitsWithSameGoal( MoveCommand ) )
			{
				if( unit_navigator_debug_inrange.GetBool() )
					DevMsg("#%d: UnitBaseNavigator::IsInRangeGoal: Not in range (dist: %f, min: %f, max: %f, tolerance: %f)\n", 
						GetOuter()->entindex(), m_fGoalDistance, GetPath()->m_fMinRange, GetPath()->m_fMaxRange, fTargetTolerance  );
				return false;
			}
		}
		else if( m_fGoalDistance < GetPath()->m_fMinRange || m_fGoalDistance > GetPath()->m_fMaxRange )
		{
			if( unit_navigator_debug_inrange.GetBool() )
				DevMsg("#%d: UnitBaseNavigator::IsInRangeGoal: Not in range (dist: %f, min: %f, max: %f)\n", 
					GetOuter()->entindex(), m_fGoalDistance, GetPath()->m_fMinRange, GetPath()->m_fMaxRange  );
			return false;
		}

		if( (GetPath()->m_iGoalFlags & GF_NOLOSREQUIRED) == 0 )
		{
			// Check LOS
			if( GetPath()->m_fnCustomLOSCheck.ptr() != Py_None )
			{
				try
				{
					if( !GetPath()->m_fnCustomLOSCheck( GetPath()->m_hTarget->BodyTarget( m_pOuter->GetAbsOrigin(), false ), GetPath()->m_hTarget->GetPyHandle() ) )
					{
						if( unit_navigator_debug_inrange.GetBool() )
							DevMsg("#%d: UnitBaseNavigator::IsInRangeGoal: No LOS to target using HasRangeAttackLOS\n", GetOuter()->entindex() );
						return false;
					}
				}
				catch( boost::python::error_already_set & )
				{
					PyErr_Print();
				}
			}
			else if( GetPath()->m_hTarget == m_pOuter->GetEnemy() )
			{
				if( !m_pOuter->HasRangeAttackLOS( GetPath()->m_hTarget->BodyTarget( m_pOuter->GetAbsOrigin(), false ), GetPath()->m_hTarget ) )
				{
					if( unit_navigator_debug_inrange.GetBool() )
						DevMsg("#%d: UnitBaseNavigator::IsInRangeGoal: No LOS to target using HasRangeAttackLOS\n", GetOuter()->entindex() );
					return false;
				}
			}
			else
			{
				// Check own los
				trace_t result;
				CTraceFilterNoNPCsOrPlayer traceFilter( GetPath()->m_hTarget, COLLISION_GROUP_NONE );
				UTIL_TraceLine( m_pOuter->EyePosition(), GetPath()->m_hTarget->BodyTarget( m_pOuter->GetAbsOrigin(), false ), MASK_BLOCKLOS_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE, &traceFilter, &result );
				if (result.fraction != 1.0f)
				{
					if( unit_navigator_debug_inrange.GetBool() )
						DevMsg("#%d: UnitBaseNavigator::IsInRangeGoal: No LOS to target\n", GetOuter()->entindex() );
					return false;
				}
			}
		}

		if( GetPath()->m_iGoalFlags & GF_REQUIREVISION )
		{
			if( !FogOfWarMgr()->PointInFOW( GetPath()->m_hTarget->EyePosition(), m_pOuter->GetOwnerNumber() ) )
			{
				if( unit_navigator_debug_inrange.GetBool() )
					DevMsg("#%d: UnitBaseNavigator::IsInRangeGoal: No vision\n", GetOuter()->entindex() );
				return false;
			}
		}
	}
	else
	{
		// Check range
		if( m_fGoalDistance < GetPath()->m_fMinRange || m_fGoalDistance > GetPath()->m_fMaxRange )
		{
			if( unit_navigator_debug_inrange.GetBool() )
				DevMsg("#%d: UnitBaseNavigator::IsInRangeGoal: Not in range (dist: %f, min: %f, max: %f)\n", 
					GetOuter()->entindex(), m_fGoalDistance, GetPath()->m_fMinRange, GetPath()->m_fMaxRange  );
			return false;
		}

		if( (GetPath()->m_iGoalFlags & GF_NOLOSREQUIRED) == 0 )
		{
			Vector vTestPos = GetPath()->m_vGoalPos;

			if( GetPath()->m_fnCustomLOSCheck.ptr() != Py_None )
			{
				try
				{
					if( !GetPath()->m_fnCustomLOSCheck( vTestPos, NULL ) )
					{
						if( unit_navigator_debug_inrange.GetBool() )
							DevMsg("#%d: UnitBaseNavigator::IsInRangeGoal: No LOS to target using HasRangeAttackLOS\n", GetOuter()->entindex() );
						return false;
					}
				}
				catch( boost::python::error_already_set & )
				{
					PyErr_Print();
				}
			}
			else
			{
				// Check own los
				trace_t result;
				CTraceFilterNoNPCsOrPlayer traceFilter( NULL, COLLISION_GROUP_NONE );
				UTIL_TraceLine( m_pOuter->EyePosition(), vTestPos, MASK_BLOCKLOS_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE, &traceFilter, &result );
				if (result.fraction != 1.0f)
				{
					if( unit_navigator_debug_inrange.GetBool() )
						DevMsg("#%d: UnitBaseNavigator::IsInRangeGoal: No LOS\n", GetOuter()->entindex() );
					return false;
				}
			}
		}

		if( GetPath()->m_iGoalFlags & GF_REQUIREVISION )
		{
			if( !FogOfWarMgr()->PointInFOW( GetPath()->m_vGoalPos, m_pOuter->GetOwnerNumber() ) )
			{
				if( unit_navigator_debug_inrange.GetBool() )
					DevMsg("#%d: UnitBaseNavigator::IsInRangeGoal: No vision\n", GetOuter()->entindex() );
				return false;
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Updates the current path.
//			Returns CHS_ATGOAL if we are at the goal waypoint.
//			Advances the current waypoint to the next one if at another waypoint.
//			Then looks at the type of the new waypoint to determine if a special
//			action needs to be taken.
//-----------------------------------------------------------------------------
CheckGoalStatus_t UnitBaseNavigator::MoveUpdateWaypoint( UnitBaseMoveCommand &MoveCommand )
{
	Vector vDir;
	UnitBaseWaypoint *pCurWaypoint = GetPath()->m_pWaypointHead;
	float waypointDist = UnitComputePathDirection2(GetAbsOrigin(), pCurWaypoint, vDir);
	
	CheckGoalStatus_t SpecialGoalStatus = CHS_NOGOAL;

	if( GetPath()->CurWaypointIsGoal() )
	{
		// Use full tolerance when blocked a bit and we have a blocker unit
		// Otherwise use waypoint tolerance (so we get as close as possible)
		float tolerance;
		if( GetBlockedStatus() > BS_LITTLE && MoveCommand.blockers.Count() > 0 && 
				MoveCommand.blockers[0].blocker && MoveCommand.blockers[0].blocker->IsUnit() && 
				MoveCommand.blockers[0].blocker->GetAbsVelocity().LengthSqr() < 16.0f * 16.0f )
		{
			tolerance = Max( GetPath()->m_waypointTolerance, GetPath()->m_fGoalTolerance );
		}
		else
		{
			tolerance = GetPath()->m_waypointTolerance, GetPath()->m_fGoalTolerance;
		}

		if( waypointDist <= Min(tolerance, GetPath()->m_fGoalTolerance) && m_fGoalDistance >= GetPath()->m_fMinRange )
		{
			if( unit_navigator_debug.GetInt() > 1 )
				DevMsg("#%d: In range goal waypoint (distance: %f, tol: %f, goaltol: %f)\n", 
						GetOuter()->entindex(), waypointDist, tolerance, GetPath()->m_fGoalTolerance );
			return CHS_ATGOAL;
		}
	}
	else
	{
		float fToleranceX = pCurWaypoint->flToleranceX + GetPath()->m_waypointTolerance;
		float fToleranceY = pCurWaypoint->flToleranceY + GetPath()->m_waypointTolerance;

		if( ( pCurWaypoint->pTo && IsCompleteInArea(pCurWaypoint->pTo, GetAbsOrigin()) ) || 
			( (fabs(GetAbsOrigin().x - pCurWaypoint->GetPos().x) < fToleranceY) && 
			(fabs(GetAbsOrigin().y - pCurWaypoint->GetPos().y) < fToleranceX) ) )
		{
			// Check special goal status
			SpecialGoalStatus = pCurWaypoint->SpecialGoalStatus;
			UnitBaseWaypoint *pNextWaypoint = pCurWaypoint->GetNext();

			if( pNextWaypoint )
			{
				if( SpecialGoalStatus == CHS_CLIMB )
				{
					Vector end = ComputeWaypointTarget( GetAbsOrigin(), pNextWaypoint );

					// Calculate climb direction.
					m_fClimbHeight = end.z - pCurWaypoint->GetPos().z;
					//Msg("%f = %f - %f (end: %f %f %f, tol: %f %f)\n", m_fClimbHeight, end.z, pCurWaypoint->GetPos().z, end.x, end.y, end.z, pNextWaypoint->flToleranceX, pNextWaypoint->flToleranceY);
					//UnitComputePathDirection2(pCurWaypoint->GetPos(), pNextWaypoint, m_vecClimbDirection);
					UnitComputePathDirection( pCurWaypoint->GetPos(), pNextWaypoint->GetPos(), m_vecClimbDirection );
				}
				else if( SpecialGoalStatus == CHS_EDGEDOWN )
				{
					m_fIgnoreNavMeshTime = gpGlobals->curtime + 5.0f;
				}
			}

			AdvancePath();
		}

	}
	if( SpecialGoalStatus != CHS_NOGOAL )
		return SpecialGoalStatus;
	return CHS_HASGOAL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector UnitBaseNavigator::ComputeWaypointTarget( const Vector &start, UnitBaseWaypoint *pEnd )
{
	Vector point1, point2, dir;

	//DirectionToVector2D( DirectionLeft(pEnd->navDir), &(dir.AsVector2D()) );
	//dir.z = 0.0f;
	// Either from south to north or east to west
	dir = pEnd->areaSlope;

	const Vector &vWayPos = pEnd->GetPos();
	if( pEnd->navDir == WEST || pEnd->navDir == EAST )
	{
		point1 = vWayPos + dir * pEnd->flToleranceX;
		point2 = vWayPos + -1 * dir * pEnd->flToleranceX;
	}
	else
	{	point1 = vWayPos + dir * pEnd->flToleranceY;
		point2 = vWayPos + -1 * dir * pEnd->flToleranceY;
	}

	if( point1 == point2 )
		return vWayPos;

	if( pEnd->SpecialGoalStatus == CHS_CLIMBDEST )
	{
		if( vWayPos.z - start.z > m_pOuter->m_fMaxClimbHeight )
		{
			if( dir.z > 0.0f ) dir *= -1;

			//float fZ = GetAbsOrigin().z
			//float fAngle = tan( m_pOuter->m_fMaxClimbHeight / pEnd->flToleranceX );
			//hookPos2 = point2 + point2 * dir * pEnd->flToleranceX / 2.0f;
		}
	}

	return UTIL_PointOnLineNearestPoint( point1, point2, start, true ); 

}

//-----------------------------------------------------------------------------
// Purpose: Advances a waypoint.
//-----------------------------------------------------------------------------
void UnitBaseNavigator::AdvancePath()
{
	GetPath()->Advance();

	m_fNextAvgDistConsideration = gpGlobals->curtime + unit_cost_history.GetFloat();
	m_fLastAvgDist = -1;

	// FIXME: When recomputing the path due being blocked, we will automatically advance path
	// while we might still be blocked. Disabled for now..
	// Just reset the waypoint distance, used by the long distance blocked check
	//ResetBlockedStatus();
	m_fLastWaypointDistance = -MAX_COORD_FLOAT;
	//m_bBlockedLongDistanceDetected = false;
}

//-----------------------------------------------------------------------------
// Purpose: Updates our current path by looking ahead.
//		    Tests if we can skip a waypoint by directly testing a route to
//			the next waypoint, resulting in a smoother path.
//			Also determines if the path is blocked.
// Based on: http://www.valvesoftware.com/publications/2009/ai_systems_of_l4d_mike_booth.pdf
//-----------------------------------------------------------------------------
bool UnitBaseNavigator::UpdateReactivePath( bool bNoRecomputePath )
{
	if( !GetPath()->m_pWaypointHead )
		return false;

	UnitBaseWaypoint *pCur, *pCurTest, *pBestWaypoint;
	float dist;
	Vector dir, testPos, endPos;
	trace_t tr;
	bool bBlocked = false;

	UTIL_TraceHull( GetAbsOrigin(), GetAbsOrigin()-Vector(0,0,MAX_TRACE_LENGTH), 
		WorldAlignMins(), WorldAlignMaxs(), m_iTestRouteMask, 
		GetOuter(), GetOuter()->CalculateIgnoreOwnerCollisionGroup(), &tr);
	testPos = tr.endpos;

	const Vector &origin = GetOuter()->EyePosition();

	float fMaxLookAhead = unit_reactivepath_maxlookahead.GetFloat();
	int nMaxLookAhead = unit_reactivepath_maxwaypointsahead.GetInt();
	pBestWaypoint = NULL;
	pCur = NULL;

	if( GetPath()->CurWaypointIsGoal() )
	{
		// Just test the head waypoint
		endPos = ComputeWaypointTarget( testPos, GetPath()->m_pWaypointHead );

		if( !TestRouteEnd( GetPath()->m_pWaypointHead ) && !TestRoute( testPos, endPos ) ) {
			bBlocked = true;
		}
	}
	else
	{
		// Build list of max waypoints we will look ahead
		pCurTest = GetPath()->m_pWaypointHead;
		for( int i = 0; i < nMaxLookAhead; i++ )
		{
			if( !pCurTest )
				break;

			dist = (origin - pCurTest->GetPos()).Length();
			if( dist > fMaxLookAhead )
				break;

			// Don't further check in case we encounter a "special travel" node like an edge
			if( pCurTest->SpecialGoalStatus == CHS_NOSIMPLIFY || pCurTest->SpecialGoalStatus == CHS_CLIMBDEST || pCurTest->SpecialGoalStatus == CHS_CLIMB )
				break;

			pCur = pCurTest; // Valid waypoint for checking up to
			pCurTest = pCurTest->GetNext();
		}

		if( !pCur )
		{
			// Just test the head waypoint
			endPos = ComputeWaypointTarget( testPos, GetPath()->m_pWaypointHead );

			if( !TestRouteEnd( GetPath()->m_pWaypointHead ) && !TestRoute( testPos, endPos ) ) {
				bBlocked = true;
			}
		}
		else
		{
			// Now test if we can directly move from our origin to that waypoint
			bBlocked = true;
			while( pCur )
			{
				endPos = ComputeWaypointTarget( testPos, pCur );

				if( !TestRouteEnd( pCur ) && !TestRoute(testPos, endPos) ) 
				{
					pCur = pCur->GetPrev();
					continue;
				}

				// Furthest waypoint we can move, so we are done. No need to test more.
				pBestWaypoint = pCur;
				break;
			}	
		

			if( pBestWaypoint )
			{
				while( GetPath()->m_pWaypointHead != pBestWaypoint)
					AdvancePath();
				bBlocked = false;
			}
		}
	}

	// Don't do the blocked test when the next node is a climb destination
	// This would like result in "being blocked" and recomputing a path will be wrong
	// at this point
	if( bBlocked && GetPath()->m_pWaypointHead->SpecialGoalStatus == CHS_CLIMBDEST )
	{
		bBlocked = false;
	}

	return bBlocked;
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float UnitBaseNavigator::ComputeWaypointDistanceAndDir( Vector &vPathDir )
{
	float fWaypointDist;
	if( m_fGoalDistance < GetPath()->m_fMinRange )
	{
		// Back off if too close
		vPathDir = GetAbsOrigin() - GetPath()->m_vGoalPos;
		vPathDir.z = 0.0f;
		fWaypointDist = VectorNormalize( vPathDir );
	}
	else
	{
		// Calculate path direction to next waypoint
		fWaypointDist = UnitComputePathDirection2( GetAbsOrigin(), GetPath()->m_pWaypointHead, vPathDir );
	}
	return fWaypointDist;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool UnitBaseNavigator::IsCompleteInArea( CNavArea *pArea, const Vector &vPos )
{
	const Vector &vWorldMins = WorldAlignMins();
	const Vector &vWorldMaxs = WorldAlignMaxs();
	const Vector &vMins = pArea->GetCorner(NORTH_WEST);
	const Vector &vMaxs = pArea->GetCorner(SOUTH_EAST);
	return (vPos.x+vWorldMins.x) >= vMins.x && (vPos.x+vWorldMaxs.x) <= vMaxs.x &&
		(vPos.y+vWorldMins.y) >= vMins.y && (vPos.y+vWorldMaxs.y) <= vMaxs.y;
}

//-----------------------------------------------------------------------------
// Purpose: Test end waypoint
//-----------------------------------------------------------------------------
bool UnitBaseNavigator::TestRouteEnd( UnitBaseWaypoint *pWaypoint )
{
	// Test if testing end waypoint
	if( pWaypoint->GetNext() != NULL )
		return false;

	// Try direct trace if we have a target as end point
	// Buildings block the nav mesh, so test route will fail
	if( GetPath()->m_hTarget )
	{
		trace_t tr;
		UTIL_TraceHull( GetAbsOrigin() + Vector(0, 0, 16), GetPath()->m_vGoalPos + Vector(0, 0, 16), 
			WorldAlignMins(), WorldAlignMaxs(), m_iTestRouteMask, 
			GetOuter(), WARS_COLLISION_GROUP_IGNORE_ALL_UNITS, &tr);
		if( tr.fraction == 1.0f || tr.m_pEnt == GetPath()->m_hTarget )
		{
			while( GetPath()->m_pWaypointHead->GetNext() )
				AdvancePath();
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Test route from start to end by using the nav mesh
//			It tests in steps of x units in the direction of end.
//			For each tested point it tests if there is a nav mesh below.
//		    It tests for two additional points using the bounding radius.
//-----------------------------------------------------------------------------
#define TEST_BENEATH_LIMIT 2000.0f
//#define DEBUG_TESTROUTE
bool UnitBaseNavigator::TestRoute( const Vector &vStartPos, const Vector &vEndPos )
{
	VPROF_BUDGET( "UnitBaseNavigator::TestRoute", VPROF_BUDGETGROUP_UNITS );
	Vector vNewStart( vStartPos );
	vNewStart.z += 16.0f;
	Vector nNewEnd( vEndPos );
	nNewEnd.z += 16.0f;

	// First do a trace. This detects if something is blocking.
	//CTraceFilterWorldOnly filter;
	trace_t tr;
	UTIL_TraceHull( vNewStart, nNewEnd, 
		WorldAlignMins(), WorldAlignMaxs(), m_iTestRouteMask, 
		GetOuter(), WARS_COLLISION_GROUP_IGNORE_ALL_UNITS, &tr);
	if( tr.fraction != 1.0f )
	{
#ifdef DEBUG_TESTROUTE
		NDebugOverlay::SweptBox( vNewStart, tr.endpos, WorldAlignMins(), WorldAlignMaxs(), QAngle(0,0,0), 255, 0, 0, 0, 0.5f );
#endif // DEBUG_TESTROUTE
		return false;
	}

#ifdef DEBUG_TESTROUTE
	NDebugOverlay::SweptBox( vNewStart, nNewEnd, WorldAlignMins(), WorldAlignMaxs(), QAngle(0,0,0), 0, 255, 0, 0, 0.5f );
#endif // DEBUG_TESTROUTE

	// Second test, take rough steps along the path and check if there is a nav area below
	CNavArea *pCur;
	float fDist, fCur, teststepsize;
	Vector vDir, vPos;
	
	vDir = nNewEnd - vNewStart;
	vDir.z = 0.0f;
	fDist = VectorNormalize(vDir);
	fCur = 16.0f;
	vPos = vNewStart;
	vPos.z += 16.0f;

	teststepsize = unit_testroute_stepsize.GetFloat();

	do
	{
		vPos += vDir * teststepsize;
		vPos.z += 16.0f;

		pCur = TheNavMesh->GetNavArea(vPos, 120.0f);
		if( !pCur )
			return false;

		vPos.z = pCur->GetZ(vPos);

		fCur += teststepsize;
	} while( fCur < fDist );

#ifdef DEBUG_TESTROUTE
	NDebugOverlay::SweptBox( vNewStart, nNewEnd, WorldAlignMins(), WorldAlignMaxs(), QAngle(0,0,0), 0, 255, 0, 0, 0.5f );
#endif // DEBUG_TESTROUTE

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnitBaseNavigator::ResetBlockedStatus()
{
	m_BlockedStatus = BS_NONE;

	NavDbgMsg("#%d ResetBlockedStatus\n", GetOuter()->entindex());
	m_fBlockedNextPositionCheck = gpGlobals->curtime + 1.0f;
	m_bBlockedLongDistanceDetected = false;
	m_fLastWaypointDistance = -MAX_COORD_FLOAT;
	m_bLowVelocityDetectionActive = false;
}

//-----------------------------------------------------------------------------
// Purpose: Update blocked status based on the last position.
//-----------------------------------------------------------------------------
void UnitBaseNavigator::UpdateBlockedStatus( UnitBaseMoveCommand &MoveCommand, const float &fWaypointDist )
{
	// Blocked status update only means something if we have a goal
	if( m_bNoPathVelocity || m_LastGoalStatus != CHS_HASGOAL || m_bLimitPositionActive )
	{
		m_BlockedStatus = BS_NONE;
		return;
	}

	// Detect low velocity situation (which means the unit is not moving from the current position)
	float fWishSpeedThreshold = MoveCommand.maxspeed * 0.35f;
	float fWishSpeed = m_vLastWishVelocity.Length2DSqr();
	float fVelocity2dSqr = GetAbsVelocity().Length2DSqr();

	if( unit_blocked_lowvel_check.GetBool() && fWishSpeed > fWishSpeedThreshold && fVelocity2dSqr < fWishSpeedThreshold )
	{
		if( !m_bLowVelocityDetectionActive )
		{
			m_fLowVelocityStartTime = gpGlobals->curtime;
			m_bLowVelocityDetectionActive = true;
		}
	}
	else
	{
		m_bLowVelocityDetectionActive = false;
		m_fLowVelocityStartTime = gpGlobals->curtime;
	}

	// Check long distance if needed
	// Per interval, we expect we at least move a proportion of the max speed we can move.
	if( m_fBlockedNextPositionCheck < gpGlobals->curtime )
	{
#if 0
		float fDistSqr = (m_vBlockedLastPosition - GetAbsOrigin()).Length2DSqr();
		float fMaxSpeedSqr = MoveCommand.maxspeed * MoveCommand.maxspeed;
		float fMinDistMovedSqr = (fMaxSpeedSqr * 3.0f * 3.0f) / 2.0f;
		m_bBlockedLongDistanceDetected = fDistSqr < fMinDistMovedSqr;
		m_fBlockedNextPositionCheck = gpGlobals->curtime + 3.0f;
		m_vBlockedLastPosition = GetAbsOrigin();
#else
		// Only perform this check if we have no target or if the target is not moving
		if( unit_blocked_longdist_check.GetBool() && fWishSpeed > fWishSpeedThreshold && ( !GetPath()->m_hTarget || GetPath()->m_hTarget->GetAbsVelocity().IsLengthLessThan( 10.0f ) ) )
		{
			if( m_fLastWaypointDistance < 0 )
			{
				m_fLastWaypointDistance = fWaypointDist;
				//m_bBlockedLongDistanceDetected = false;
			}
			else
			{
				m_bBlockedLongDistanceDetected = ( m_fLastWaypointDistance - fWaypointDist ) < ( (MoveCommand.maxspeed * 1.0f) / 4.0f );
				m_fLastWaypointDistance = Min( m_fLastWaypointDistance, fWaypointDist );
			}

			//if( m_bBlockedLongDistanceDetected )
			//	NavDbgMsg("#%d UpdateBlockedStatus: Long distance blocked detected.\n", GetOuter()->entindex());
		}
		else //if( GetPath()->m_hTarget )
		{
			// Target is moving, so always reset the distance to avoid incorrect blocked detection
			m_fLastWaypointDistance = -MAX_COORD_FLOAT;
			m_bBlockedLongDistanceDetected = false;
		}

		m_fBlockedNextPositionCheck = gpGlobals->curtime + 1.0f;
#endif // 0
	}

	if( (gpGlobals->curtime - m_fLowVelocityStartTime) < 1.0f && !m_bBlockedLongDistanceDetected )
	{
		m_BlockedStatus = BS_NONE;
		return;
	}

	// Only execute this part if blocked...
	if( m_BlockedStatus == BS_NONE )
	{
		m_iBlockedPathRecomputations = 0;
		m_fBlockedStartTime = gpGlobals->curtime;
	}

	float fBlockedTime = gpGlobals->curtime - m_fBlockedStartTime;
	if( fBlockedTime < 2.5f )
		m_BlockedStatus = BS_LITTLE;
	else if( fBlockedTime < 6.0f )
		m_BlockedStatus = BS_MUCH;
	else if( fBlockedTime < 16.f )
		m_BlockedStatus = BS_STUCK;
	else
		m_BlockedStatus = BS_GIVEUP;
}

// Goals
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool UnitBaseNavigator::SetGoal( Vector &destination, float goaltolerance, int goalflags, bool avoidenemies )
{
	bool bResult = FindPath( GOALTYPE_POSITION, destination, goaltolerance, goalflags, 0, 0, NULL, avoidenemies );
	if( !bResult )
	{
		GetPath()->m_iGoalType = GOALTYPE_NONE; // Keep path around for querying the information about the last path
	}
	return bResult;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool UnitBaseNavigator::SetGoalTarget( CBaseEntity *pTarget, float goaltolerance, int goalflags, bool avoidenemies )
{
	if( !pTarget )
	{
#ifdef ENABLE_PYTHON
		PyErr_SetString(PyExc_Exception, "SetGoalTarget: target is None" );
		throw boost::python::error_already_set(); 
#endif // ENABLE_PYTHON
		return false;
	}
	bool bResult = FindPath( GOALTYPE_TARGETENT, pTarget->WorldSpaceCenter(), goaltolerance, goalflags, 0, 0, pTarget, avoidenemies );
	if( !bResult )
	{
		GetPath()->m_iGoalType = GOALTYPE_NONE; // Keep path around for querying the information about the last path
	}
	return bResult;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool UnitBaseNavigator::SetGoalInRange( Vector &destination, float maxrange, float minrange, float goaltolerance, int goalflags, bool avoidenemies )
{
	bool bResult = FindPath( GOALTYPE_POSITION_INRANGE, destination, goaltolerance, goalflags, minrange, maxrange, NULL, avoidenemies );
	if( !bResult )
	{
		GetPath()->m_iGoalType = GOALTYPE_NONE; // Keep path around for querying the information about the last path
	}
	return bResult;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool UnitBaseNavigator::SetGoalTargetInRange( CBaseEntity *pTarget, float maxrange, float minrange, float goaltolerance, int goalflags, bool avoidenemies )
{
	if( !pTarget )
	{
#ifdef ENABLE_PYTHON
		PyErr_SetString(PyExc_Exception, "SetGoalTargetInRange: target is None" );
		throw boost::python::error_already_set(); 
#endif // ENABLE_PYTHON
		return false;
	}

	// Find a path
	bool bResult = FindPath( GOALTYPE_TARGETENT_INRANGE, pTarget->EyePosition(), goaltolerance, goalflags, minrange, maxrange, pTarget, avoidenemies );
	if( !bResult )
	{
		GetPath()->m_iGoalType = GOALTYPE_NONE; // Keep path around for querying the information about the last path
	}
	return bResult;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool UnitBaseNavigator::SetVectorGoal( const Vector &dir, float targetDist, float minDist, bool fShouldDeflect )
{
	Vector result;

	if ( FindVectorGoal( &result, dir, targetDist, minDist ) )
		return SetGoal( result );
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Update In range path information
//-----------------------------------------------------------------------------
void UnitBaseNavigator::UpdateGoalInRange( float maxrange, float minrange, UnitBasePath *pPath )
{
	if( !pPath ) 
		pPath = GetPath();

	if( pPath->m_iGoalType != GOALTYPE_POSITION_INRANGE && pPath->m_iGoalType != GOALTYPE_TARGETENT_INRANGE )
	{
#ifdef ENABLE_PYTHON
		PyErr_SetString(PyExc_Exception, "UnitBaseNavigator::UpdateGoalInRange: Invalid goal type" );
		throw boost::python::error_already_set(); 
#else
		Warning("UnitBaseNavigator::UpdateGoalInRange: Invalid goal type\n");
#endif // ENABLE_PYTHON
		return;
	}

	pPath->m_fMaxRange = maxrange;
	pPath->m_fMinRange = minrange;
}

//-----------------------------------------------------------------------------
// Purpose: Update path target information
//-----------------------------------------------------------------------------
void UnitBaseNavigator::UpdateGoalTarget( CBaseEntity *pTarget, UnitBasePath *pPath )
{
	if( !pPath ) 
		pPath = GetPath();

	if( pPath->m_iGoalType != GOALTYPE_TARGETENT && pPath->m_iGoalType != GOALTYPE_TARGETENT_INRANGE )
	{
#ifdef ENABLE_PYTHON
		PyErr_SetString(PyExc_Exception, "UnitBaseNavigator::UpdateGoalTarget: Invalid goal type" );
		throw boost::python::error_already_set(); 
#else
		Warning("UnitBaseNavigator::UpdateGoalTarget: Invalid goal type\n");
#endif // ENABLE_PYTHON
		return;
	}

	pPath->m_hTarget = pTarget;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnitBaseNavigator::UpdateGoalInfo( void )
{
	if( GetPath()->m_iGoalType == GOALTYPE_NONE )
		return;

	// Get distance and direction
	if( GetPath()->m_hTarget )
	{
		if( GetPath()->m_iGoalFlags & GF_USETARGETDIST )
		{
			m_fGoalDistance = m_pOuter->EnemyDistance( GetPath()->m_hTarget );
			m_fGoalDistance = Max(0.0f, m_fGoalDistance); // Negative distance makes no sense
		}
		else
		{
			m_vGoalDir = GetPath()->m_hTarget->GetAbsOrigin() - GetAbsOrigin();
			m_vGoalDir.z = 0.0f;
			m_fGoalDistance = VectorNormalize( m_vGoalDir );
		}
	}
	else
	{
		m_vGoalDir = GetPath()->m_vGoalPos - GetAbsOrigin();
		m_vGoalDir.z = 0.0f;
		m_fGoalDistance = VectorNormalize( m_vGoalDir );
	}
	
}

//-----------------------------------------------------------------------------
// Purpose: Last computed target distance
//-----------------------------------------------------------------------------
float UnitBaseNavigator::GetGoalDistance( void )
{
	// Might be called before we did an update (e.g. when a path is restored).
	// In this case, update the goal distance + direction here
	if( m_fGoalDistance == -1 )
	{
		UpdateGoalInfo();
	}
	return m_fGoalDistance;
}

// Path finding
//-----------------------------------------------------------------------------
// Purpose: Creates, builds and finds a new path.
//-----------------------------------------------------------------------------
bool UnitBaseNavigator::FindPathInternal( UnitBasePath *pPath, int goaltype, const Vector &vDestination, float fGoalTolerance, int iGoalFlags, float fMinRange, float fMaxRange, CBaseEntity *pTarget, bool bAvoidEnemies )
{
	if( unit_navigator_debug.GetBool() )
	{
		DevMsg( "#%d UnitNavigator: Finding new path (goaltype: ",  GetOuter()->entindex() );

		// Print goal
		switch( goaltype )
		{
		case GOALTYPE_NONE:
			DevMsg("NONE");
			break;
		case GOALTYPE_INVALID:
			DevMsg("INVALID");
			break;
		case GOALTYPE_POSITION:
			DevMsg("POSITION");
			break;
		case GOALTYPE_TARGETENT:
			DevMsg("TARGET ENTITY");
			break;
		case GOALTYPE_POSITION_INRANGE:
			DevMsg("POSITION IN RANGE");
			break;
		case GOALTYPE_TARGETENT_INRANGE:
			DevMsg("TARGET ENTITY IN RANGE");
			break;
		default:
			DevMsg("INVALID GOALTYPE");
			break;
		}

		DevMsg(", flags: ");

		// print flags
		if( iGoalFlags & GF_NOCLEAR )
			DevMsg("GF_NOCLEAR ");
		if( iGoalFlags & GF_REQTARGETALIVE )
			DevMsg("GF_REQTARGETALIVE ");
		if( iGoalFlags & GF_USETARGETDIST )
			DevMsg("GF_USETARGETDIST ");
		if( iGoalFlags & GF_NOLOSREQUIRED )
			DevMsg("GF_NOLOSREQUIRED ");
		if( iGoalFlags & GF_REQUIREVISION )
			DevMsg("GF_REQUIREVISION ");
		if( iGoalFlags & GF_OWNERISTARGET )
			DevMsg("GF_OWNERISTARGET ");
		if( iGoalFlags & GF_DIRECTPATH )
			DevMsg("GF_DIRECTPATH ");

		DevMsg(")\n");
	}

	Reset();

	m_LastGoalStatus = CHS_HASGOAL;

	const Vector &vOrigin = GetAbsOrigin();

	bool bPathReused = false;
#if 0
	if( pPath->m_pWaypointHead && (pPath->m_vGoalPos - vDestination).Length2D() < 512.0f &&
		(pPath->m_pWaypointHead->GetPos() - vOrigin).Length2D() < 512.0f )
	{
		// Reuse path
		bPathReused = true;
		NavDbgMsg("#%d FindPath: reusing previous path\n", GetOuter()->entindex());
	}
#endif // 0

	pPath->m_vStartPosition = vOrigin;
	pPath->m_iGoalType = goaltype;
	pPath->m_vGoalPos = vDestination;
	pPath->m_fGoalTolerance = fGoalTolerance;
	pPath->m_waypointTolerance = GetEntityBoundingRadius( m_pOuter );
	pPath->m_iGoalFlags = iGoalFlags;
	pPath->m_fMinRange = fMinRange;
	pPath->m_fMaxRange = fMaxRange;
	pPath->m_hTarget = pTarget;
	pPath->m_bSuccess = false;
	pPath->m_bAvoidEnemies = bAvoidEnemies;

	if( !bPathReused )
	{
		if( pPath->m_iGoalType == GOALTYPE_POSITION ||
				pPath->m_iGoalType == GOALTYPE_TARGETENT )
			return DoFindPathToPos( pPath );
		else if( pPath->m_iGoalType == GOALTYPE_POSITION_INRANGE ||
				pPath->m_iGoalType == GOALTYPE_TARGETENT_INRANGE )
			return DoFindPathToPosInRange( pPath );
	}
	else
	{
		// Just need to update the last waypoint
		pPath->m_pWaypointHead->GetLast()->SetPos(GetPath()->m_vGoalPos);
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Finds a path to the goal position.
//-----------------------------------------------------------------------------
bool UnitBaseNavigator::DoFindPathToPos( UnitBasePath *pPath )
{
	VPROF_BUDGET( "UnitBaseNavigator::DoFindPathToPos", VPROF_BUDGETGROUP_UNITS );

	// Used for throttling too many path recomputations by this navigator
	m_fLastPathRecomputation = gpGlobals->curtime;

	m_iCurPathRecomputations++;

	pPath->SetWaypoint(NULL);

	UnitBaseWaypoint *waypoints = BuildRoute( pPath );
	if( !waypoints )
		return false;

	pPath->SetWaypoint(waypoints);
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Creates, builds and finds a new path and sets it as new path for 
//			the unit.
//-----------------------------------------------------------------------------
bool UnitBaseNavigator::FindPath( int goaltype, const Vector &vDestination, float fGoalTolerance, int iGoalFlags, float fMinRange, float fMaxRange, CBaseEntity *pTarget, bool bAvoidEnemies )
{
	if( FindPathInternal( GetPath(), goaltype, vDestination, fGoalTolerance, iGoalFlags, fMinRange, fMaxRange, pTarget, bAvoidEnemies ) )
	{
		if( unit_reactivepath.GetBool() )
			UpdateReactivePath( true );
		return true;
	}
	return false;
}

#ifdef ENABLE_PYTHON
//-----------------------------------------------------------------------------
// Purpose: Creates, builds and finds a new path and returns it as result.
//-----------------------------------------------------------------------------
boost::python::object UnitBaseNavigator::FindPathAsResult( int goaltype, const Vector &vDestination, float fGoalTolerance, int goalflags, float fMinRange, float fMaxRange, CBaseEntity *pTarget, bool bAvoidEnemies )
{
	boost::python::object refPath = SrcPySystem()->RunT<boost::python::object>( SrcPySystem()->Get("UnitBasePath", unit_helper), boost::python::object() );
	UnitBasePath *pPath = boost::python::extract<UnitBasePath *>(refPath);

	FindPathInternal( pPath, goaltype, vDestination, fGoalTolerance, goalflags, fMinRange, fMaxRange, pTarget, bAvoidEnemies );

	return refPath;
}
#endif // ENABLE_PYTHON

// Route buiding
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
UnitBaseWaypoint *UnitBaseNavigator::BuildLocalPath( UnitBasePath *pPath, const Vector &vGoalPos )
{
	if( !unit_route_local_paths.GetBool() )
		return NULL;

	if( GetAbsOrigin().DistTo(vGoalPos) < 300 )
	{
		//Do a simple trace
		trace_t tr;
		UTIL_TraceHull(GetAbsOrigin()+Vector(0,0,16.0), vGoalPos+Vector(0,0,16.0), 
			WorldAlignMins(), WorldAlignMaxs(), m_iTestRouteMask, GetOuter(), GetOuter()->CalculateIgnoreOwnerCollisionGroup(), &tr);
		if( tr.DidHit() && (!GetPath()->m_hTarget || !tr.m_pEnt || tr.m_pEnt != GetPath()->m_hTarget) )
			return NULL;

		NavDbgMsg("#%d BuildLocalPath: builded local route\n", GetOuter()->entindex());
		UnitBaseWaypoint *pWayPoint = new UnitBaseWaypoint(vGoalPos);
		pWayPoint->flToleranceX = pPath->m_waypointTolerance + 1.0f;
		pWayPoint->flToleranceY = pPath->m_waypointTolerance + 1.0f;

		pPath->m_bIsDirectPath = true;
		return pWayPoint;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#define WAYPOINT_UP_Z 8.0f
UnitBaseWaypoint *UnitBaseNavigator::BuildWayPointsFromRoute( UnitBasePath *pPath, CNavArea *goalArea, UnitBaseWaypoint *pWayPoint, int prevdir )
{
	if( !goalArea || !goalArea->GetParent() )
		return pWayPoint;

	CNavArea *fromArea;
	Vector closest, hookPos, hookPos2, center;
	float halfWidth, margin, hmargin;
	NavDirType dir, fromdir;
	int fromfromdir;
	NavTraverseType how;
	UnitBaseWaypoint *pFromAreaWayPoint, *pGoalAreaWayPoint;

	// Calculate position of the new waypoint
	fromArea = goalArea->GetParent();
	center = fromArea->GetCenter();
	how = goalArea->GetParentHow();
	if( how >= NUM_DIRECTIONS )
	{
		Warning("BuildWayPointsFromRoute: Unsupported navigation type");
		return pWayPoint;
	}
	fromdir = (NavDirType)how;
	dir = OppositeDirection( fromdir );
	goalArea->ComputeSemiPortal( fromArea, dir, &hookPos, &halfWidth );
	fromArea->ComputeSemiPortal( goalArea, fromdir, &hookPos2, &halfWidth );

	// Compute margin
	//const Vector &vWorldSize = WorldAlignSize();
	//margin = sqrt( vWorldSize.x*vWorldSize.x+vWorldSize.y*vWorldSize.y );
	margin = GetEntityBoundingRadius( m_pOuter ) * 2.0f;
	hmargin = margin/2.0f;

	// Shouldn't be needed with the new tolerance settings
	fromfromdir = fromArea->GetParent() ? OppositeDirection( (NavDirType)fromArea->GetParentHow() ) : -1;

	// Get tolerances
	float fTolerance = Max( halfWidth - hmargin, 0.0f );

	// Calculate tolerance + new hookpos


#if 0 // TODO: Recheck
	// If the path through the connected areas are all facing in the same direction, then we just place the waypoint in the portal
	// Otherwise we place two waypoints in the "to" and "from" area to ensure we walk into the next area correctly
	// Otherwise we might get stuck around corners
	if( !pWayPoint || (fromfromdir == dir && prevdir == dir) )
	{
		hookPos.z = fromArea->GetZ( hookPos ) + WAYPOINT_UP_Z;
		pNewWayPoint = new UnitBaseWaypoint( hookPos );
		pNewWayPoint->pFrom = fromArea;
		pNewWayPoint->pTo = goalArea;
		pNewWayPoint->navDir = dir;
		if( pWayPoint )
			pNewWayPoint->SetNext(pWayPoint);
		pWayPoint = pNewWayPoint;

		if( dir == WEST || dir == EAST )
		{
			pNewWayPoint->flToleranceX = pPath->m_waypointTolerance + 1.0f;
			pNewWayPoint->flToleranceY = Min(fGoalTolX, fFromTolX);
		}
		else
		{
			pNewWayPoint->flToleranceX = Min(fGoalTolY, fFromTolY);
			pNewWayPoint->flToleranceY = pPath->m_waypointTolerance + 1.0f;
		}
	}
	else 
#endif // 0
	{
		// Move the waypoint in the goal area
		Vector waypointPos = hookPos;
		switch( fromdir )
		{
		case NORTH:
			waypointPos.y -= fromArea->GetSizeY() > hmargin ? hmargin : fromArea->GetSizeY()/2.0f;
			break;
		case SOUTH:
			waypointPos.y += fromArea->GetSizeY() > hmargin ? hmargin : fromArea->GetSizeY()/2.0f;
			break;
		case WEST:
			waypointPos.x -= fromArea->GetSizeX() > hmargin ? hmargin : fromArea->GetSizeX()/2.0f;
			break;
		case EAST:
			waypointPos.x += fromArea->GetSizeX() > hmargin ? hmargin : fromArea->GetSizeX()/2.0f;
			break;
		}

		waypointPos.z = goalArea->GetZ( waypointPos ) + WAYPOINT_UP_Z;

		pGoalAreaWayPoint = new UnitBaseWaypoint( waypointPos );
		pGoalAreaWayPoint->pFrom = fromArea;
		pGoalAreaWayPoint->pTo = goalArea;
		pGoalAreaWayPoint->navDir = dir;
		if( pWayPoint )
			pGoalAreaWayPoint->SetNext(pWayPoint);

		if( dir == WEST || dir == EAST )
		{
			pGoalAreaWayPoint->flToleranceX = fTolerance;
			pGoalAreaWayPoint->flToleranceY = pPath->m_waypointTolerance;

			pGoalAreaWayPoint->areaSlope = (goalArea->GetCorner( SOUTH_WEST ) - goalArea->GetCorner( NORTH_WEST ));
		}
		else
		{
			pGoalAreaWayPoint->flToleranceX = pPath->m_waypointTolerance;
			pGoalAreaWayPoint->flToleranceY = fTolerance;

			pGoalAreaWayPoint->areaSlope = (goalArea->GetCorner( SOUTH_EAST ) - goalArea->GetCorner( SOUTH_WEST ));
		}

		// Construct another waypoint in the 'from' area
		waypointPos = hookPos2;
		switch( fromdir )
		{
		case NORTH:
			waypointPos.y += goalArea->GetSizeY() > hmargin ? hmargin : goalArea->GetSizeY()/2.0f;
			break;
		case SOUTH:
			waypointPos.y -= goalArea->GetSizeY() > hmargin ? hmargin : goalArea->GetSizeY()/2.0f;
			break;
		case WEST:
			waypointPos.x += goalArea->GetSizeX() > hmargin ? hmargin : goalArea->GetSizeX()/2.0f;
			break;
		case EAST:
			waypointPos.x -= goalArea->GetSizeX() > hmargin ? hmargin : goalArea->GetSizeX()/2.0f;
			break;
		}
		waypointPos.z = fromArea->GetZ( waypointPos ) + WAYPOINT_UP_Z;

		pFromAreaWayPoint = new UnitBaseWaypoint( waypointPos );
		pFromAreaWayPoint->pFrom = fromArea;
		pFromAreaWayPoint->pTo = goalArea;
		pFromAreaWayPoint->navDir = dir;
		pFromAreaWayPoint->SetNext( pGoalAreaWayPoint );

		if( dir == WEST || dir == EAST )
		{
			pFromAreaWayPoint->flToleranceX = fTolerance;
			pFromAreaWayPoint->flToleranceY = pPath->m_waypointTolerance;

			pFromAreaWayPoint->areaSlope = (fromArea->GetCorner( SOUTH_WEST ) - fromArea->GetCorner( NORTH_WEST ));
		}
		else
		{
			pFromAreaWayPoint->flToleranceX = pPath->m_waypointTolerance;
			pFromAreaWayPoint->flToleranceY = fTolerance;

			pFromAreaWayPoint->areaSlope = (fromArea->GetCorner( SOUTH_EAST ) - fromArea->GetCorner( SOUTH_WEST ));
		}

		pGoalAreaWayPoint->areaSlope.NormalizeInPlace();
		pFromAreaWayPoint->areaSlope.NormalizeInPlace();

		// Add special markers if needed
		if( !fromArea->IsContiguous( goalArea ) )
		{
			float heightdiff = fromArea->ComputeAdjacentConnectionHeightChange( goalArea );

			if( heightdiff > 0 )
			{
				// FIXME: sometimes incorrectly marked as climb, while m_fMaxClimbHeight == 0.
				//			In this case that should never happen, so it seems the check is slightly 
				//			different here than in the pathfind cost function.
				if( m_pOuter->m_fMaxClimbHeight != 0 )
				{
					pFromAreaWayPoint->SpecialGoalStatus = CHS_CLIMB;
					pGoalAreaWayPoint->SpecialGoalStatus = CHS_CLIMBDEST;

					pFromAreaWayPoint->SetPos( hookPos2 );

					//Vector point1, point2;
					if( dir == WEST || dir == EAST )
					{
						pFromAreaWayPoint->flToleranceY = 2.0f;
						pGoalAreaWayPoint->flToleranceY = 2.0f;
					}
					else
					{
						pFromAreaWayPoint->flToleranceX = 2.0f;
						pGoalAreaWayPoint->flToleranceX = 2.0f;
					}


				}
			}
			else
			{
				// TODO
				pFromAreaWayPoint->SpecialGoalStatus = CHS_EDGEDOWN;
				pGoalAreaWayPoint->SpecialGoalStatus = CHS_EDGEDOWNDEST;
			}
		}

		// Return the waypoint in the from area
		pWayPoint = pFromAreaWayPoint;
	}

	return BuildWayPointsFromRoute( pPath, fromArea, pWayPoint, dir );  

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
UnitBaseWaypoint *UnitBaseNavigator::BuildNavAreaPath( UnitBasePath *pPath, const Vector &vGoalPos )
{
	if( !unit_route_navmesh_paths.GetBool() )
		return NULL;

#if 0
	CNavArea *startArea, *goalArea, *closestArea;

	// Use GetAbsOrigin here. Nav area selection falls back to nearest nav.
	// If we only use GetNavArea, then prefer EyeOffset (because some nav areas might have a decent distance from the ground).
	// In case GetNearestNavArea is used, always use AbsOrigin, because otherwise you might select an undesired nav area based
	// on distance.
	const Vector &vStart = GetAbsOrigin(); 

	startArea = TheNavMesh->GetNavArea( vStart, 120.0f, true /* checkBlocked */ );
	if( !startArea )
		startArea = TheNavMesh->GetNearestNavArea( vStart );
	goalArea = TheNavMesh->GetNavArea( vGoalPos, 120.0f, true /* checkBlocked */ );

	if( unit_navigator_debug.GetInt() == 2 )
	{
		NDebugOverlay::Box( vStart, -Vector(8, 8, 8), Vector(8, 8, 8), 255, 0, 0, true, 5.0f );
		NDebugOverlay::Box( vGoalPos, -Vector(8, 8, 8), Vector(8, 8, 8), 0, 255, 0, true, 5.0f );

		if( startArea ) 
			NDebugOverlay::Box( startArea->GetCenter(), -Vector(8, 8, 8), Vector(8, 8, 8), 255, 0, 255, true, 5.0f );
		if( goalArea ) 
			NDebugOverlay::Box( goalArea->GetCenter(), -Vector(8, 8, 8), Vector(8, 8, 8), 128, 200, 0, true, 5.0f );
	}

	// Only build route if we have both a start and goal area, otherwise too expensive
	if( !startArea )
	{
		NavDbgMsg( "#%d BuildNavAreaPath: No navigation area found for start position\n", GetOuter()->entindex() );
		if( unit_route_requirearea.GetBool() ) 
			return new UnitBaseWaypoint(vGoalPos);
	}

	// If the startArea is the goalArea we are done.
	if( goalArea && startArea == goalArea )
	{
		NavDbgMsg( "#%d BuildNavAreaPath: Start area is goal area, going direct\n", GetOuter()->entindex() );
		return new UnitBaseWaypoint(vGoalPos);
	}

	// Build route from navigation mesh
	bool bUsingCachedPath = false;
	CUtlSymbol unittype( GetOuter()->GetUnitType() );
	if( unit_allow_cached_paths.GetBool() && startArea && goalArea && CNavArea::IsPathCached( unittype, startArea->GetID(), goalArea->GetID() ) )
	{
		NavDbgMsg( "#%d BuildNavAreaPath: Using cached path for start area %d and goal area %d\n", 
			GetOuter()->entindex(), startArea->GetID(), goalArea->GetID() );
		closestArea = CNavArea::GetCachedClosestArea();
		bUsingCachedPath = true;
	}
	else
	{
		// TODO: Maybe consider the max range in the path finding?
		//float fMaxRange = pPath->m_fMaxRange;
		//if( pPath->m_hTarget )
		//	fMaxRange += pPath->m_hTarget->CollisionProp()->BoundingRadius2D();

		UnitShortestPathCost costFunc( m_pOuter );
		UnitNavAreaBuildPath<UnitShortestPathCost>( startArea, goalArea, &vGoalPos, costFunc, &closestArea );
	}

	if( closestArea )
	{
		if( !goalArea || closestArea != goalArea )
		{
			NavDbgMsg( "#%d BuildNavAreaPath: Found end area is not the goal area. Going to closest area instead.\n", GetOuter()->entindex() );
			if( unit_navigator_debug.GetInt() == 2 )
				NDebugOverlay::Box( closestArea->GetCenter(), -Vector(8, 8, 8), Vector(8, 8, 8), 0, 0, 255, true, 5.0f );
		}
		
		if( !bUsingCachedPath && goalArea )
		{
			NavDbgMsg( "#%d BuildNavAreaPath: Caching path from area %d to %d with closest area %d.\n", GetOuter()->entindex(),
				startArea->GetID(), goalArea->GetID(), closestArea->GetID() );
			CNavArea::SetCachedPath( unittype, startArea->GetID(), goalArea->GetID(), closestArea->GetID() );
		}

		UnitBaseWaypoint *pEnd = new UnitBaseWaypoint( vGoalPos );
		pEnd->flToleranceX = pPath->m_waypointTolerance + 1.0f;
		pEnd->flToleranceY = pPath->m_waypointTolerance + 1.0f;
		return BuildWayPointsFromRoute( pPath, closestArea, pEnd );
	}

	// Fall back
	Warning( "#%d BuildNavAreaPath: falling back to a direct path to goal\n", GetOuter()->entindex() );
	return new UnitBaseWaypoint( vGoalPos );
#else
	CRecastMesh *pNavMesh = GetRecastNavMesh();

	const Vector &vStart = GetAbsOrigin(); 
	UnitBaseWaypoint *pFoundPath = pNavMesh->FindPath( vStart, vGoalPos );
	if( pFoundPath )
		return pFoundPath;

	return new UnitBaseWaypoint( vGoalPos );
#endif // 0
}

//-----------------------------------------------------------------------------
// Purpose: Tries to build a route using either a direct trace or the nav mesh.
//-----------------------------------------------------------------------------
UnitBaseWaypoint *UnitBaseNavigator::BuildRoute( UnitBasePath *pPath )
{
	UnitBaseWaypoint *waypoints;

	// Special case: build a direct path
	if( pPath->m_iGoalFlags & GF_DIRECTPATH )
	{
		pPath->m_bIsDirectPath = true;
		return new UnitBaseWaypoint( pPath->m_vGoalPos );
	}

	// In some cases, units should arrive from a specific side
	// In this case add an extra waypoint at the "entrance", which can't
	// be skipped by the unit
	IUnit *pUnit = pPath->m_hTarget ? pPath->m_hTarget->GetIUnit() : NULL;
	if( pUnit && pUnit->HasEnterOffset() )
	{
		Vector vOffset = pUnit->GetEnterOffset();
		VectorYawRotate( vOffset, pPath->m_hTarget->GetAbsAngles()[YAW], vOffset );
		Vector vEnterPoint = pPath->m_hTarget->GetAbsOrigin() + vOffset;

		NavDbgMsg("#%d BuildNavAreaPath: Building route to target enter point\n", GetOuter()->entindex());

		// "Expensive": use nav mesh
		waypoints = BuildNavAreaPath( pPath, vEnterPoint );
		
		UnitBaseWaypoint *pEndWaypoint = new UnitBaseWaypoint( pPath->m_vGoalPos );
		pEndWaypoint->SpecialGoalStatus = CHS_NOSIMPLIFY;
		UnitBaseWaypoint *pLast = waypoints->GetLast();
		pLast->SetNext( pEndWaypoint );
		return waypoints;
	}
	else
	{
		// Cheap: try to do trace from start to goal
		waypoints = BuildLocalPath( pPath, pPath->m_vGoalPos );
		if( waypoints ) 
			return waypoints;

		// Expensive: use nav mesh
		waypoints = BuildNavAreaPath( pPath, pPath->m_vGoalPos );
		if( waypoints )
			return waypoints;
	}

	// Fallback to a direct path
	pPath->m_bIsDirectPath = true;
	return new UnitBaseWaypoint( pPath->m_vGoalPos );
}

#ifdef ENABLE_PYTHON
//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void UnitBaseNavigator::SetPath( boost::python::object path )
{
	if( path.ptr() == Py_None )
	{
		// Install the default path object
		m_refPath = unit_helper.attr("UnitBasePath")();
		m_pPath = boost::python::extract<UnitBasePath *>(m_refPath);
		m_pPath->m_vGoalPos = GetAbsOrigin();
		return;
	}

	m_pPath = boost::python::extract<UnitBasePath *>(path);
	m_refPath = path;
}
#endif // ENABLE_PYTHON

#if 0
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void UnitBaseNavigator::CalculateDestinationInRange( Vector *pResult, const Vector &vGoalPos, float minrange, float maxrange)
{
	Vector dir;
	float dist = (vGoalPos - GetAbsOrigin()).Length2D();
	if( dist < minrange )
	{
		dir = GetAbsOrigin() - vGoalPos;
		VectorNormalize(dir);
		FindVectorGoal(pResult, dir, minrange);
	}
	else if( dist > maxrange )
	{
		dir = vGoalPos - GetAbsOrigin();
		VectorNormalize(dir);
		FindVectorGoal(pResult, dir, dist-maxrange);
	}
	else
	{
		*pResult = GetAbsOrigin();
	}
}


#endif 

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool UnitBaseNavigator::FindVectorGoal( Vector *pResult, const Vector &dir, float targetDist, float minDist )
{
	Vector testLoc = GetAbsOrigin() + ( dir * targetDist );

	CNavArea *pArea = TheNavMesh->GetNearestNavArea(testLoc);
	if( !pArea )
		return false;

	if( !pArea->Contains(testLoc) ) {
		pArea->GetClosestPointOnArea(testLoc, pResult);
		return true;
	}
	testLoc.z = pArea->GetZ(testLoc);
	*pResult = testLoc;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void UnitBaseNavigator::CalculateDeflection( const Vector &start, const Vector &dir, const Vector &normal, Vector *pResult )
{
	Vector temp;
	CrossProduct( dir, normal, temp );
	CrossProduct( normal, temp, *pResult );
	VectorNormalize( *pResult );
}

//-----------------------------------------------------------------------------
// Purpose: Draw the list of waypoints for debugging
//-----------------------------------------------------------------------------
void UnitBaseNavigator::DrawDebugRouteOverlay()
{
	if( GetPath()->m_iGoalType != GOALTYPE_NONE )
	{
		UnitBaseWaypoint *pWaypoint = GetPath()->m_pWaypointHead;
		if( !pWaypoint)
			return;

		NDebugOverlay::Line(GetAbsOrigin(), pWaypoint->GetPos(), 0, 0, 255, true, 0);
		while( pWaypoint )
		{
			int r, g, b;
			switch( pWaypoint->SpecialGoalStatus )
			{
			case CHS_NOGOAL:
			case CHS_HASGOAL:
			case CHS_ATGOAL:
			case CHS_FAILED:
				r = 0;
				g = 255;
				b = 0;
				break;
			case CHS_CLIMB:
				r = 255;
				g = 0;
				b = 0;
				break;
			default:
				r = 255;
				g = 255;
				b = 0;
				break;
			}
			NDebugOverlay::Box(pWaypoint->GetPos(), Vector(-3,-3,-3),Vector(3,3,3), r, g, b, true,0);
			if( pWaypoint->GetNext() )
				NDebugOverlay::Line(pWaypoint->GetPos(), pWaypoint->GetNext()->GetPos(), 0, 0, 255, true, 0);
			pWaypoint = pWaypoint->GetNext();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnitBaseNavigator::DrawDebugInfo()
{
	float fDensity, fRadius;
	int i, j;

	if( unit_navigator_debugoverlay_ent.GetInt() != -1 && unit_navigator_debugoverlay_ent.GetInt() != GetOuter()->entindex() )
		return;

	fRadius = GetEntityBoundingRadius(m_pOuter);

	// Draw consider entities
	for( i = 0; i < m_iConsiderSize; i++ )
	{
		if( m_ConsiderList[i] )
			NDebugOverlay::EntityBounds(m_ConsiderList[i], 0, 255, 0, 50, 0 );
	}	

	// Draw test positions + density info
	fDensity = 0;
	for( j = 0; j < m_iUsedTestDirections; j++ )
	{
		fDensity = m_DensitySums[ j ];

		if( unit_navigator_debug_showavgvels.GetBool() )
		{
			NDebugOverlay::HorzArrow( GetLocalOrigin(), GetLocalOrigin() + m_vAverageVelocities[j], 
								2.0f, fDensity*255, (1.0f-Min<float>(1.0f, Max<float>(0.0,fDensity)))*255, 0, 200, true, 0 );
		}
		else
		{
			NDebugOverlay::HorzArrow( GetLocalOrigin(), m_vTestPositions[j], 
								2.0f, fDensity*255, (1.0f-Min<float>(1.0f, Max<float>(0.0,fDensity)))*255, 0, 200, true, 0 );
			NDebugOverlay::Text( m_vTestPositions[j], UTIL_VarArgs("%f", fDensity), false, 0.0 );
		}
	}

	// Draw velocities
	NDebugOverlay::HorzArrow(GetAbsOrigin(), GetAbsOrigin() + m_vDebugVelocity, 
						4.0f, 0, 0, 255, 200, true, 0);

	NDebugOverlay::HorzArrow(GetAbsOrigin(), GetAbsOrigin() + m_vTestDirections[m_iDebugBestDir] * 96.0f, 
						4.0f, 0, 255, 255, 200, true, 0);

	

	// Draw out blocking direction
	NDebugOverlay::HorzArrow(GetAbsOrigin(), GetAbsOrigin() + m_vBlockingDirection * 64.0, 
						4.0f, 0, 255, 0, 200, true, 0);
	NDebugOverlay::HorzArrow(GetAbsOrigin(), GetAbsOrigin() + -m_vBlockingDirection * 64.0, 
						4.0f, 0, 255, 0, 200, true, 0);

	m_pOuter->EntityText( 0, UTIL_VarArgs("BestCost: %.2f\n", m_fDebugLastBestCost), 0, 255, 0, 0, 255 );
	m_pOuter->EntityText( 1, UTIL_VarArgs("FinalVel: %.2f %.2f %.2f ( %.2f )\n", 
		m_vDebugVelocity.x, m_vDebugVelocity.y, m_vDebugVelocity.z, m_vDebugVelocity.Length2D()), 0, 255, 0, 0, 255 );
	m_pOuter->EntityText( 2, UTIL_VarArgs("Density: %f\n", m_fLastBestDensity), 0, 255, 0, 0, 255 );
	m_pOuter->EntityText( 3, UTIL_VarArgs("BoundingRadius: %.2f\n", GetEntityBoundingRadius(m_pOuter)), 0, 255, 0, 0, 255 );
	m_pOuter->EntityText( 4, UTIL_VarArgs("Threshold: %f\n", THRESHOLD), 0, 255, 0, 0, 255 );
	m_pOuter->EntityText( 5, UTIL_VarArgs("DiscomfortWeight: %f\n", m_fDiscomfortWeight), 0, 255, 0, 0, 255 );
	m_pOuter->EntityText( 6, UTIL_VarArgs("m_BlockedStatus: %d\n", m_BlockedStatus), 0, 255, 0, 0, 255 );
	m_pOuter->EntityText( 7, UTIL_VarArgs("m_GoalStatus: %d\n", m_LastGoalStatus), 0, 255, 0, 0, 255 );
	m_pOuter->EntityText( 7, UTIL_VarArgs("m_vBlockingDirection: %.2f %.2f %.2f (%.2f)\n", 
		m_vBlockingDirection.x, m_vBlockingDirection.y, m_vBlockingDirection.z, m_vBlockingDirection.Length()), 0, 255, 0, 0, 255 );
	
}
