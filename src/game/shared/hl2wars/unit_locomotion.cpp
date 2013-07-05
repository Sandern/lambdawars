//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "unit_locomotion.h"
#include "movevars_shared.h"
#include "coordsize.h"

#ifndef CLIENT_DLL
#include "nav_mesh.h"
#include "nav_area.h"
#endif // CLIENT_DLL

#include "vphysics/object_hash.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar unit_nolocomotion("unit_nolocomotion", "0", FCVAR_CHEAT|FCVAR_REPLICATED);

#define	MAX_CLIP_PLANES	5

float UnitComputePathDirection( const Vector &start, const Vector &end, Vector &pDirection )
{
	VectorSubtract( end, start, pDirection );
	pDirection.z = 0.0f;
	return Vector2DNormalize( pDirection.AsVector2D() );
}

enum UnitStepGroundTest_t
{
	STEP_DONT_CHECK_GROUND = 0,
	STEP_ON_VALID_GROUND,
	STEP_ON_INVALID_GROUND,
};

#define	LOCAL_STEP_SIZE 48.0f // 16.0f // 8 // 16

//#define DEBUG_MOVEMENT_VELOCITY

#ifdef DEBUG_MOVEMENT_VELOCITY
	static bool bMovement = false;
	static bool bVelocity = false;
#endif // DEBUG_MOVEMENT_VELOCITY

//-----------------------------------------------------------------------------
// Nav Trace Filter
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CTraceFilterUnitNav::CTraceFilterUnitNav( CUnitBase *pProber, const IHandleEntity *passedict, int collisionGroup) : 
	CTraceFilterSimple( passedict, collisionGroup ),
	m_pProber(pProber)
{
	m_bCheckCollisionTable = g_EntityCollisionHash->IsObjectInHash( pProber );
}

//-----------------------------------------------------------------------------
bool CTraceFilterUnitNav::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	if ( pHandleEntity == m_pProber )
		return false;

	//CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
	//if ( !pEntity )
	//	return false;
	CBaseEntity *pEntity = (CBaseEntity *)pHandleEntity;

	if ( pEntity->IsNavIgnored() )
		return false;

	if ( m_bCheckCollisionTable )
	{
		if ( g_EntityCollisionHash->IsObjectPairInHash( m_pProber, pEntity ) )
			return false;
	}

	//if ( m_pProber->ShouldProbeCollideAgainstEntity( pEntity ) == false )
	//	return false;

	return CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask );
}


//-----------------------------------------------------------------------------
// Movement class
//-----------------------------------------------------------------------------
#ifdef ENABLE_PYTHON
UnitBaseLocomotion::UnitBaseLocomotion( boost::python::object outer ) : UnitComponent(outer)
{
	surfacefriction = 1.0;
	unitsolidmask = MASK_NPCSOLID;  // FIXME: Use PhysicsSolidMaskForEntity?
	stepsize = 18.0f;

	acceleration = 10.0f;
	airacceleration = 10.0f;
	worldfriction = 4.0f;
	stopspeed = 100.0f;

	mv = NULL;

	m_pTraceListData = NULL;
}
#endif // ENABLE_PYTHON

#define TICK_INTERVAL			(gpGlobals->interval_per_tick)


#define TIME_TO_TICKS( dt )		( (int)( 0.5f + (float)(dt) / TICK_INTERVAL ) )
#define TICKS_TO_TIME( t )		( TICK_INTERVAL *( t ) )
#define ROUND_TO_TICKS( t )		( TICK_INTERVAL * TIME_TO_TICKS( t ) )
#define TICK_NEVER_THINK		(-1)

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void UnitBaseLocomotion::PerformMovement( UnitBaseMoveCommand &mv )
{
	VPROF_BUDGET( "UnitBaseLocomotion::PerformMovement", VPROF_BUDGETGROUP_UNITS );

	if( unit_nolocomotion.GetBool() )
		return;

	SetupMove(mv);
	Move(mv.interval, mv);
	FinishMove(mv);
}

//-----------------------------------------------------------------------------
// Purpose
//-----------------------------------------------------------------------------
void UnitBaseLocomotion::PerformMovementFacingOnly( UnitBaseMoveCommand &move_command )
{
	VPROF_BUDGET( "UnitBaseLocomotion::PerformMovementFacingOnly", VPROF_BUDGETGROUP_UNITS );

	if( unit_nolocomotion.GetBool() )
		return;

	SetupMove(move_command);
	
	mv = &move_command;

	mv->interval = move_command.interval;
	mv->outwishvel.Init();

	MoveFacing();

	FinishMove(move_command);
}


//-----------------------------------------------------------------------------
// Purpose: Setup/Finish a move
//-----------------------------------------------------------------------------
void UnitBaseLocomotion::SetupMove( UnitBaseMoveCommand &mv )
{
#ifdef CLIENT_DLL
	mv.velocity = m_pOuter->GetAbsVelocity();
	mv.origin = m_pOuter->GetNetworkOrigin();
	mv.viewangles = m_pOuter->GetAbsAngles();
#else
	mv.viewangles = m_pOuter->GetAbsAngles();
	mv.velocity = m_pOuter->GetAbsVelocity();
	mv.origin = m_pOuter->GetAbsOrigin();
#endif // CLIENT_DLL

	// Clamp maxspeed
	mv.maxspeed = MIN(mv.maxspeed, 5000.0f);

	SetupMovementBounds( mv );
}

void UnitBaseLocomotion::FinishMove( UnitBaseMoveCommand &mv )
{
#ifdef CLIENT_DLL
	m_pOuter->SetAbsVelocity( mv.velocity );
	m_pOuter->SetLocalAngles( mv.viewangles );
	m_pOuter->m_vecNetworkOrigin = mv.origin;
	m_pOuter->SetLocalOrigin( mv.origin );
#else
	m_pOuter->SetAbsVelocity( mv.velocity );
	m_pOuter->SetLocalAngles( mv.viewangles );

	// Set origin after setting the velocity, in case the trigger touch function changes the velocity.
	//UTIL_SetOrigin(m_pOuter, mv.origin, true);
	m_pOuter->SetAbsOrigin( mv.origin );
	m_pOuter->PhysicsTouchTriggers();

	// check to see if our ground entity has changed
	// NOTE: This is to detect changes in ground entity as the movement code has optimized out
	// ground checks.  So now we have to do a simple recheck to make sure we detect when we've 
	// stepped onto a new entity.
	if ( m_pOuter->GetFlags() & FL_ONGROUND )
	{
		m_pOuter->PhysicsStepRecheckGround();
	}
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Acceleration
//-----------------------------------------------------------------------------
bool UnitBaseLocomotion::CanAccelerate()
{
	return true;
}

void UnitBaseLocomotion::Accelerate( Vector& wishdir, float wishspeed, float accel)
{
	int i;
	float addspeed, accelspeed, currentspeed;

	// This gets overridden because some games (CSPort) want to allow dead (observer) m_pOuters
	// to be able to move around.
	if ( !CanAccelerate() )
		return;

	// See if we are changing direction a bit
	currentspeed = mv->velocity.Dot(wishdir);

	// Reduce wishspeed by the amount of veer.
	addspeed = wishspeed - currentspeed;

	// If not going to add any speed, done.
	if (addspeed <= 0)
		return;

	// Determine amount of accleration.
	accelspeed = accel * mv->interval * wishspeed * surfacefriction;

	// Cap at addspeed
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	// Adjust velocity.
	for (i=0 ; i<3 ; i++)
	{
		mv->velocity[i] += accelspeed * wishdir[i];	
	}
}

void UnitBaseLocomotion::AirAccelerate( Vector& wishdir, float wishspeed, float accel )
{
	int i;
	float addspeed, accelspeed, currentspeed;
	float wishspd;

	wishspd = wishspeed;

	// Cap speed
	if (wishspd > 30)
		wishspd = 30;

	// Determine veer amount
	currentspeed = mv->velocity.Dot(wishdir);

	// See how much to add
	addspeed = wishspd - currentspeed;

	// If not adding any, done.
	if (addspeed <= 0)
		return;

	// Determine acceleration speed after acceleration
	accelspeed = accel * wishspeed * mv->interval * surfacefriction;

	// Cap it
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	// Adjust pmove vel.
	for (i=0 ; i<3 ; i++)
	{
		mv->velocity[i] += accelspeed * wishdir[i];
		mv->outwishvel[i] += accelspeed * wishdir[i];
	}
}

//-----------------------------------------------------------------------------
// Determine if unit is in water, on ground, etc.
//-----------------------------------------------------------------------------
void UnitBaseLocomotion::CategorizePosition( void )
{
	surfacefriction = 1.0;

	if( mv->velocity.z > 100.0f ) // Skip check, otherwise we might end up running the walk code (which is ground based only)
	{
		SetGroundEntity( NULL ); 
		//DevMsg("No ground ent due large upward velocity\n");
	}
	else
	{
		trace_t pm;
		TraceUnitBBox( mv->origin, mv->origin-Vector(0, 0, 2.0f), unitsolidmask, m_pOuter->GetCollisionGroup(), pm );
		if( !pm.m_pEnt || pm.plane.normal[2] < 0.7 )
			SetGroundEntity( NULL );
		else
			GetOuter()->SetGroundEntity( pm.m_pEnt );
	}
}

//-----------------------------------------------------------------------------
// If I were to stop moving, how much distance would I walk before I'm halted?
//-----------------------------------------------------------------------------
float UnitBaseLocomotion::GetStopDistance()
{

	float	speed, newspeed, control;
	float	friction;
	float	drop;
	float	distance;
	int		i;
	
	// Calculate speed
	if( mv->velocity.IsValid() )
		speed = VectorLength( mv->velocity );
	else
		speed = 0.0f;

	distance = 0.0f;

	for( i = 0; i < 1000; i++ )
	{
		drop = 0;

		// apply ground friction
		if (m_pOuter->GetGroundEntity() != NULL)  // On an entity that is the ground
		{
			friction = worldfriction * surfacefriction;

			// Bleed off some speed, but if we have less than the bleed
			//  threshold, bleed the threshold amount.
			control = (speed < stopspeed) ? stopspeed : speed;

			// Add the amount to the drop amount.
			drop += control*friction*mv->interval;
		}

		// scale the velocity
		newspeed = speed - drop;
		if (newspeed < 0)
			newspeed = 0;

		distance += newspeed * mv->interval;

		speed = newspeed;

		if( speed <= 0.1f )
			break;
	}

 	//mv->outwishvel -= (1.f-newspeed) * mv->velocity;

	return distance;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool UnitBaseLocomotion::ShouldWalk()
{
	// Velocity NULL and on the ground
	if( !VectorCompare(mv->velocity, vec3_origin) || 
		!VectorCompare(GetOuter()->GetBaseVelocity(), vec3_origin) ||
		mv->jump )
		return false;
	return true;
}

//-----------------------------------------------------------------------------
// Walking
//-----------------------------------------------------------------------------
void UnitBaseLocomotion::WalkMove( void )
{
	int i;

	Vector wishvel;
	float fmove, smove;
	Vector wishdir;
	float wishspeed;

	Vector dest;
	Vector forward, right, up;

	AngleVectors (mv->viewangles, &forward, &right, &up);  // Determine movement angles

	CHandle< CBaseEntity > oldground;
	oldground = GetOuter()->GetGroundEntity();

	// Copy movement amounts
	fmove = mv->forwardmove;
	smove = mv->sidemove;

	// Zero out z components of movement vectors
	if ( forward[2] != 0 )
	{
		forward[2] = 0;
		VectorNormalize( forward );
	}

	if ( right[2] != 0 )
	{
		right[2] = 0;
		VectorNormalize( right );
	}

	for (i=0 ; i<2 ; i++)       // Determine x and y parts of velocity
		wishvel[i] = forward[i]*fmove + right[i]*smove;

	wishvel[2] = 0;             // Zero out z part of velocity

	VectorCopy (wishvel, wishdir);   // Determine maginitude of speed of move
	wishspeed = VectorNormalize(wishdir);

	//
	// Clamp to server defined max speed
	//
	if ((wishspeed != 0.0f) && (wishspeed > mv->maxspeed))
	{
		VectorScale (wishvel, mv->maxspeed/wishspeed, wishvel);
		wishspeed = mv->maxspeed;
	}

	// Set pmove velocity
	mv->velocity[2] = 0;
	Accelerate ( wishdir, wishspeed, acceleration );
	mv->velocity[2] = 0;

	if( ShouldWalk() )
	{
		UpdateBlockerNoMove();
		return;
	}

	// first try just moving to the destination	
	//dest[0] = mv->origin[0] + mv->velocity[0]*mv->interval;
	//dest[1] = mv->origin[1] + mv->velocity[1]*mv->interval;
	//dest[2] = mv->origin[2];

	// Add in any base velocity to the current velocity.
	//VectorAdd (mv->velocity, m_pOuter->GetBaseVelocity(), mv->velocity );

	// 2d movement (xy)
	//GroundMove(mv->origin, dest, unitsolidmask, 0);
	GroundMove();

	// Now pull the base velocity back out.   Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
	//VectorSubtract( mv->velocity, m_pOuter->GetBaseVelocity(), mv->velocity );

#ifdef DEBUG_MOVEMENT_VELOCITY
	static Vector vLastOrigin;

	if( fmove || smove )
	{
		if( !bMovement )
		{
			bMovement = true;
			Msg("%f Movement changed %f %f vel %f \n", gpGlobals->curtime, fmove, smove, mv->velocity.Length());
		}
	}
	else
	{
		if( bMovement )
		{
			bMovement = false;
			Msg("%f Movement changed to zero %f %f vel %f. Stop distance: %f\n", gpGlobals->curtime, fmove, smove, mv->velocity.Length(), GetStopDistance());
			vLastOrigin = mv->origin;
		}
	}

	if( mv->velocity.Length() > 25.0f )
	{
		if( !bVelocity )
		{
			bVelocity = true;
			Msg("%f velocity changed %f\n", gpGlobals->curtime, mv->velocity.Length());
		}
	}
	else
	{
		if( bVelocity )
		{
			bVelocity = false;
			Msg("%f velocity changed to zero %f. Distance: %f\n", gpGlobals->curtime, mv->velocity.Length(), (mv->origin - vLastOrigin).Length());
		}
	}
#endif // DEBUG_MOVEMENT_VELOCITY

	mv->stopdistance = GetStopDistance();

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnitBaseLocomotion::UpdateBlockerNoMove()
{
	trace_t trace;
	TraceUnitBBox( mv->origin, mv->origin+Vector(0,0,1), unitsolidmask, m_pOuter->GetCollisionGroup(), trace );
	mv->m_hBlocker = trace.m_pEnt;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnitBaseLocomotion::AirMove( void )
{
	int			i;
	Vector		wishvel;
	float		fmove, smove;
	Vector		wishdir;
	float		wishspeed;
	Vector forward, right, up;
	Vector dest;

	AngleVectors (mv->viewangles, &forward, &right, &up);  // Determine movement angles

	// Copy movement amounts
	fmove = mv->forwardmove;
	smove = mv->sidemove;

	// Zero out z components of movement vectors
	forward[2] = 0;
	right[2]   = 0;
	VectorNormalize(forward);  // Normalize remainder of vectors
	VectorNormalize(right);    // 

	for (i=0 ; i<2 ; i++)       // Determine x and y parts of velocity
		wishvel[i] = forward[i]*fmove + right[i]*smove;
	wishvel[2] = 0;             // Zero out z part of velocity

	VectorCopy (wishvel, wishdir);   // Determine maginitude of speed of move
	wishspeed = VectorNormalize(wishdir);

	//
	// clamp to server defined max speed
	//
	if ( wishspeed != 0 && (wishspeed > mv->maxspeed))
	{
		VectorScale (wishvel, mv->maxspeed/wishspeed, wishvel);
		wishspeed = mv->maxspeed;
	}

	AirAccelerate( wishdir, wishspeed, airacceleration );

	if( ShouldWalk() )
	{
		UpdateBlockerNoMove();
		return;
	}

	// first try just moving to the destination	
	dest = mv->origin + mv->velocity*mv->interval;

	CUtlVector<CBaseEntity *> ignoredEntities;
	trace_t trace;
	for( i = 0; i < 8; i++ )
	{
		TraceUnitBBox( mv->origin, mv->origin+Vector(0,0,1), unitsolidmask, m_pOuter->GetCollisionGroup(), trace );
		if( !trace.startsolid || !trace.m_pEnt || 
			(!trace.m_pEnt->AllowNavIgnore() && trace.m_pEnt->GetMoveType() != MOVETYPE_VPHYSICS) )
			break;
		ignoredEntities.AddToTail( trace.m_pEnt );
		trace.m_pEnt->SetNavIgnore();
		//m_pTraceListData->m_aEntityList.FindAndRemove( trace.m_pEnt );
		//m_pTraceListData->m_nEntityCount--;	
	}

	if( trace.startsolid && !(trace.m_pEnt && trace.m_pEnt->AllowNavIgnore()) )
	{
		// Apply hack
		TraceUnitBBox( mv->origin + Vector(0,0,64.0f), mv->origin, unitsolidmask, m_pOuter->GetCollisionGroup(), trace );
		mv->origin = trace.endpos;
		DevMsg("#%d Applying unstuck hack at position %f %f %f (ent: %s)\n", m_pOuter->entindex(), mv->origin.x, mv->origin.y, mv->origin.z, 
			trace.m_pEnt ? trace.m_pEnt->GetClassname() : "null");
	}

	trace_t pm;
	UnitTryMove(&pm);

	// Clear nav ignored entities	
	for ( i = 0; i < ignoredEntities.Count(); i++ )
	{
		ignoredEntities[i]->ClearNavIgnore();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnitBaseLocomotion::FullWalkMove( )
{
	StartGravity();

	CategorizePosition();

	// Clear ground entity, so we will do an air move.
	if( mv->jump )
		HandleJump();

	if( GetGroundEntity() == NULL || (GetOuter()->GetFlags() & FL_FLY) )
	{	
		AirMove();
	}
	else
	{
		WalkMove();
	}

	FinishGravity();

	// If we are on ground, no downward velocity.
	if( GetGroundEntity() != NULL )
		mv->velocity.z = 0.0f;
}

//-----------------------------------------------------------------------------

float Unit_ClampYaw( float yawSpeedPerSec, float current, float target, float time )
{
	if (current != target)
	{
		float speed = yawSpeedPerSec * time;
		float move = target - current;

		if (target > current)
		{
			if (move >= 180)
				move = move - 360;
		}
		else
		{
			if (move <= -180)
				move = move + 360;
		}

		if (move > 0)
		{// turning to the npc's left
			if (move > speed)
				move = speed;
		}
		else
		{// turning to the npc's right
			if (move < -speed)
				move = -speed;
		}

		return anglemod(current + move);
	}

	return target;
}

//-----------------------------------------------------------------------------
// Face
//-----------------------------------------------------------------------------
void UnitBaseLocomotion::MoveFacing( void )
{
	float current = anglemod( mv->viewangles.y );
	float ideal = anglemod( mv->idealviewangles.y );

	float newYaw = Unit_ClampYaw( mv->yawspeed * 10.0, current, ideal, mv->interval );

	if( newYaw != current )
		mv->viewangles.y = newYaw;
}

//-----------------------------------------------------------------------------
// Main move function
//-----------------------------------------------------------------------------
void UnitBaseLocomotion::Move( float interval, UnitBaseMoveCommand &move_command )
{
	VPROF_BUDGET( "UnitBaseLocomotion::Move", VPROF_BUDGETGROUP_UNITS );

	mv = &move_command;

	mv->interval = interval;
	mv->outwishvel.Init();

	Friction();
	FullWalkMove();
	MoveFacing();
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
//#define UNIT_DEBUGSTEP 
void UnitBaseLocomotion::GroundMove()
{
	CUtlVector<CBaseEntity *> ignoredEntities;
	trace_t trace;
	Vector stepEnd;
	float fIntervalStepSize;
	int i;

	// Clear current blocker
	mv->m_hBlocker = NULL;

	// Calculate the max stepsize for this interval
	// Assume we can move up/down stepsize per 48.0
	fIntervalStepSize = MAX(stepsize, (stepsize*(mv->interval*mv->maxspeed/48.0)));

	// Raise ourself stepsize
	// Combine with testing the start position.
	// Set blocker to ignore if allowed.
	stepEnd = mv->origin;
	stepEnd.z = mv->origin.z + fIntervalStepSize;
	for( i = 0; i < 8; i++ )
	{
		TraceUnitBBox( mv->origin, stepEnd, unitsolidmask, m_pOuter->GetCollisionGroup(), trace );
		if( !trace.startsolid || !trace.m_pEnt || 
			(!trace.m_pEnt->AllowNavIgnore() && trace.m_pEnt->GetMoveType() != MOVETYPE_VPHYSICS) )
			break;
		ignoredEntities.AddToTail( trace.m_pEnt );
		trace.m_pEnt->SetNavIgnore();
		//m_pTraceListData->m_aEntityList.FindAndRemove( trace.m_pEnt );
		//m_pTraceListData->m_nEntityCount--;	
	}
	mv->origin = trace.endpos;

	if( trace.startsolid )
	{
		// Apply hack
		TraceUnitBBox( mv->origin + Vector(0,0,64.0f), mv->origin, unitsolidmask, m_pOuter->GetCollisionGroup(), trace );
		mv->origin = trace.endpos;
	}

#if !defined(CLIENT_DLL) && defined( UNIT_DEBUGSTEP )
	NDebugOverlay::SweptBox( mv->origin, trace.endpos, WorldAlignMins(), WorldAlignMaxs(), QAngle(0,0,0), 64, 64, 64, 0, 5 );
#endif // !(CLIENT_DLL) && defined( UNIT_DEBUGSTEP )

	// Run basic velocity
	trace_t pm;
	UnitTryMove(&pm);

#if !defined(CLIENT_DLL) && defined( UNIT_DEBUGSTEP )
	NDebugOverlay::Box( mv->origin, WorldAlignMins(), WorldAlignMaxs(), 64, 64, 0, 0, 5 );
#endif // !(CLIENT_DLL) && defined( UNIT_DEBUGSTEP )

	// Stay on ground
	// Since we just move according to our velocity we might end up in the air a bit (instead of using something like stepmove).
	// Just put us on the ground.
	stepEnd = mv->origin;
	stepEnd.z = mv->origin.z - fIntervalStepSize*2;
	TraceUnitBBox( mv->origin, stepEnd, unitsolidmask, m_pOuter->GetCollisionGroup(), trace );
	mv->origin = trace.endpos;

#if !defined(CLIENT_DLL) && defined( UNIT_DEBUGSTEP )
	NDebugOverlay::SweptBox( mv->origin, trace.endpos, WorldAlignMins(), WorldAlignMaxs(), QAngle(0,0,0), 0, 64, 64, 0, 5 );
#endif // !(CLIENT_DLL) && defined( UNIT_DEBUGSTEP )

	// Clear nav ignored entities	
	for ( i = 0; i < ignoredEntities.Count(); i++ )
	{
		ignoredEntities[i]->ClearNavIgnore();
	}

#if !defined(CLIENT_DLL) && defined( UNIT_DEBUGSTEP )
	m_pOuter->EntityText(0, UTIL_VarArgs("vel: %f %f %f (%f)\n", mv->velocity.x, mv->velocity.y, mv->velocity.z, mv->velocity.Length()), 0.2f);
#endif // !(CLIENT_DLL) && defined( UNIT_DEBUGSTEP )
}

#if 0
ConVar unit_try_ignore("unit_try_ignore", "1");
ConVar unit_clip_velocity("unit_clip_velocity", "1");
//-----------------------------------------------------------------------------
// Checks a ground-based movement
// NOTE: The movement will be based on an *actual* start position and
// a *desired* end position; it works this way because the ground-based movement
// is 2 1/2D, and we may end up on a ledge above or below the actual desired endpoint.
//-----------------------------------------------------------------------------
bool UnitBaseLocomotion::GroundMove( const Vector &vecActualStart, const Vector &vecDesiredEnd, 
									  unsigned int collisionMask, unsigned flags )
{
	VPROF_BUDGET( "UnitBaseLocomotion::GroundMove", VPROF_BUDGETGROUP_UNITS );

	Vector		dir;
	float		d;
	int			numplanes;
	Vector		planes[MAX_CLIP_PLANES];
	Vector		primal_velocity, original_velocity, new_velocity;
	int i, j, k;

	mv->m_hBlocker				= NULL;
	mv->blocker_hitpos			= vec3_origin;
	mv->outstepheight			= 0;

	Vector vecMoveDir;
	mv->totaldistance = UnitComputePathDirection( vecActualStart, vecDesiredEnd, vecMoveDir );
	if (mv->totaldistance == 0.0f)
	{
		return true;
	}

	//  Take single steps towards the goal
	float distClear = 0;

	UnitCheckStepArgs_t checkStepArgs;
	UnitCheckStepResult_t checkStepResult;

	checkStepArgs.vecStart				= vecActualStart;
	checkStepArgs.vecStepDir			= vecMoveDir;
	checkStepArgs.stepSize				= 0;
	checkStepArgs.stepHeight			= stepsize;
	checkStepArgs.stepDownMultiplier	= 1.0f;
	checkStepArgs.minStepLanding		= WorldAlignSize().x * 0.3333333f;
	checkStepArgs.collisionMask			= collisionMask;

	checkStepResult.endPoint = vecActualStart;
	checkStepResult.hitNormal = vec3_origin;
	checkStepResult.pBlocker = NULL;

	bool bTryNavIgnore = ( ( vecActualStart - GetLocalOrigin() ).Length2DSqr() < 0.1 && fabsf(vecActualStart.z - GetLocalOrigin().z) < checkStepArgs.stepHeight * 0.5 );

	CUtlVector<CBaseEntity *> ignoredEntities;

	// TODO: Cleanup/debug this code.
	for (;;)
	{
		float flStepSize = MIN( LOCAL_STEP_SIZE, mv->totaldistance - distClear );
		if ( flStepSize < 0.001 )
			break;

		checkStepArgs.stepSize = flStepSize;

		if( unit_try_ignore.GetBool() )
		{
		// First ignore anything we should ignore
		for ( i = 0; i < 16; i++ )
		{
			CheckStep( checkStepArgs, &checkStepResult );

			if ( !bTryNavIgnore || !checkStepResult.pBlocker || !checkStepResult.fStartSolid )
				break;

			if ( checkStepResult.pBlocker->GetMoveType() != MOVETYPE_VPHYSICS && !checkStepResult.pBlocker->IsUnit() )
				break;

			if( checkStepResult.pBlocker->IsUnit() && !checkStepResult.pBlocker->AllowNavIgnore() )
				break;

			// Only permit pass through of objects initially embedded in
			if ( vecActualStart != checkStepArgs.vecStart )
			{
				bTryNavIgnore = false;
				break;
			}

			// Only allow move away from physics objects
			if ( checkStepResult.pBlocker->GetMoveType() == MOVETYPE_VPHYSICS )
			{
				Vector vMoveDir = vecDesiredEnd - vecActualStart;
				VectorNormalize( vMoveDir );

				Vector vObstacleDir = (checkStepResult.pBlocker->WorldSpaceCenter() - GetOuter()->WorldSpaceCenter() );
				VectorNormalize( vObstacleDir );

				if ( vMoveDir.Dot( vObstacleDir ) >= 0 )
					break;
			}
			
			//Msg("Ignoring %s\n", checkStepResult.pBlocker->GetClassname());
			ignoredEntities.AddToTail( checkStepResult.pBlocker );
			checkStepResult.pBlocker->SetNavIgnore();
		}
		}

#if 0
		// Copy original velocity
		VectorCopy (mv->velocity, original_velocity);
		VectorCopy (mv->velocity, primal_velocity);
		numplanes = 0;

		int			bumpcount, numbumps;
		numbumps = 4;
		Vector vecAbsVelocity = mv->velocity;

		if( unit_clip_velocity.GetBool() && checkStepResult.pBlocker ) 
		{
			for (bumpcount=0 ; bumpcount<numbumps ; bumpcount++)
			{
				if (vecAbsVelocity == vec3_origin)
					break;

				vecMoveDir = vecAbsVelocity;
				VectorNormalize(vecMoveDir);
				checkStepArgs.vecStepDir = vecMoveDir;
				CheckStep( checkStepArgs, &checkStepResult );

				if( checkStepResult.fStartSolid )
				{
					mv->velocity = vec3_origin;
					break;
				}

				if( ( checkStepResult.endPoint - checkStepArgs.vecStart ).Length2D() >= flStepSize )
					break;

				if( checkStepResult.endPoint != checkStepArgs.vecStart )
				{
					//distClear += ( checkStepResult.endPoint - checkStepArgs.vecStart ).Length2D();
					//checkStepArgs.vecStart = checkStepResult.endPoint;
					mv->velocity = vecAbsVelocity;
					numplanes = 0;
				}

				// clipped to another plane
				if (numplanes >= MAX_CLIP_PLANES)
				{	// this shouldn't really happen
					mv->velocity = vec3_origin;
					break;
				}

				VectorCopy (checkStepResult.hitNormal, planes[numplanes]);
				numplanes++;

				// modify original_velocity so it parallels all of the clip planes
				if ( m_pOuter->GetMoveType() == MOVETYPE_WALK && (!(GetFlags() & FL_ONGROUND) || m_pOuter->GetFriction()!=1) )	// relfect m_pOuter velocity
				{
					for ( i = 0; i < numplanes; i++ )
					{
						if ( planes[i][2] > 0.7  )
						{// floor or slope
							ClipVelocity( original_velocity, planes[i], new_velocity, 1 );
							VectorCopy( new_velocity, original_velocity );
						}
						else
						{
							ClipVelocity( original_velocity, planes[i], new_velocity, 1.0 + sv_bounce.GetFloat() * (1-m_pOuter->GetFriction()) );
						}
					}

					VectorCopy( new_velocity, vecAbsVelocity );
					VectorCopy( new_velocity, original_velocity );
				}
				else
				{
					for (i=0 ; i<numplanes ; i++)
					{
						ClipVelocity (original_velocity, planes[i], new_velocity, 1);
						for (j=0 ; j<numplanes ; j++)
							if (j != i)
							{
								if (DotProduct (new_velocity, planes[j]) < 0)
									break;	// not ok
							}
						if (j == numplanes)
							break;
					}
					
					if (i != numplanes)
					{	
						// go along this plane
						VectorCopy (new_velocity, vecAbsVelocity);
					}
					else
					{	
						// go along the crease
						if (numplanes != 2)
						{
			//				Msg( "clip velocity, numplanes == %i\n",numplanes);
							mv->velocity = vecAbsVelocity;
							break;
						}
						CrossProduct (planes[0], planes[1], dir);
						d = DotProduct (dir, vecAbsVelocity);
						VectorScale (dir, d, vecAbsVelocity);
					}

					//
					// if original velocity is against the original velocity, stop dead
					// to avoid tiny oscillations in sloping corners
					//
					if (DotProduct (vecAbsVelocity, primal_velocity) <= 0)
					{
						mv->velocity = vec3_origin;
						break;
					}
				}
			}
		}
#endif // 0

		//Msg("Blocked: %d (%s), start solid? %d\n", checkStepResult.pBlocker, (checkStepResult.pBlocker ? checkStepResult.pBlocker->GetClassname() : NULL), checkStepResult.fStartSolid);
		// If we're being blocked by something, move as close as we can and stop
		if ( checkStepResult.pBlocker )
		{
			distClear += ( checkStepResult.endPoint - checkStepArgs.vecStart ).Length2D();
			break;
		}

		float dz = checkStepResult.endPoint.z - checkStepArgs.vecStart.z;
		if ( dz < 0 )
		{
			dz = 0;
		}

		mv->outstepheight += dz;
		distClear += flStepSize;
		checkStepArgs.vecStart = checkStepResult.endPoint;
	}

	for ( i = 0; i < ignoredEntities.Count(); i++ )
	{
		ignoredEntities[i]->ClearNavIgnore();
	}

	mv->origin = checkStepResult.endPoint;

	if (checkStepResult.pBlocker)
	{
		mv->m_hBlocker				= checkStepResult.pBlocker;	
		mv->blocker_hitpos			= checkStepResult.hitPos;
		mv->blocker_dir				= vecMoveDir;
		return false;
	}
	return true;
}
#endif // 0

//-----------------------------------------------------------------------------
// CheckStep() is a fundamentally 2D operation!	vecEnd.z is ignored.
// We can step up one StepHeight or down one StepHeight from vecStart
//-----------------------------------------------------------------------------
//#define UNIT_DEBUGSTEP 
bool UnitBaseLocomotion::CheckStep( const UnitCheckStepArgs_t &args, UnitCheckStepResult_t *pResult )
{
	//VPROF( "UnitBaseLocomotion::CheckStep" );
	VPROF_BUDGET( "UnitBaseLocomotion::CheckStep", VPROF_BUDGETGROUP_UNITS );

	Vector vecEnd;
	//unsigned collisionMask = args.collisionMask;
	VectorMA( args.vecStart, args.stepSize, args.vecStepDir, vecEnd );

	pResult->endPoint = args.vecStart;
	pResult->fStartSolid = false;
	pResult->hitPos = vec3_origin;
	pResult->hitNormal = vec3_origin;
	pResult->pBlocker = NULL;

	// This is fundamentally a 2D operation; we just want the end
	// position in Z to be no more than a step height from the start position
	Vector stepStart( args.vecStart.x, args.vecStart.y, args.vecStart.z /*+ UNIT_MOVE_HEIGHT_EPSILON*/ );
	Vector stepEnd( vecEnd.x, vecEnd.y, args.vecStart.z /*+ UNIT_MOVE_HEIGHT_EPSILON*/ );

#if !defined(CLIENT_DLL) && defined( UNIT_DEBUGSTEP )
	NDebugOverlay::Line( stepStart, stepEnd, 255, 255, 255, true, 5 );
	NDebugOverlay::Cross3D( stepEnd, 32, 255, 255, 255, true, 5 );
#endif // !(CLIENT_DLL) && defined( UNIT_DEBUGSTEP )

	trace_t trace;

	TraceUnitBBox( stepStart, stepEnd, unitsolidmask, m_pOuter->GetCollisionGroup(), trace );

	if (trace.startsolid || (trace.fraction < 1))
	{
		// Either the entity is starting embedded in the world, or it hit something.
		// Raise the box by the step height and try again
		trace_t stepTrace;

		if ( !trace.startsolid )
		{
#if !defined(CLIENT_DLL) && defined( UNIT_DEBUGSTEP )
			NDebugOverlay::Box( trace.endpos, WorldAlignMins(), WorldAlignMaxs(), 64, 64, 64, 0, 5 );
#endif // !(CLIENT_DLL) && defined( UNIT_DEBUGSTEP )

			// Advance to first obstruction point
			stepStart = trace.endpos;

			// Trace up to locate the maximum step up in the space
			Vector stepUp( stepStart );
			stepUp.z += args.stepHeight;
			TraceUnitBBox( stepStart, stepUp, unitsolidmask, m_pOuter->GetCollisionGroup(), stepTrace );

#if !defined(CLIENT_DLL) && defined( UNIT_DEBUGSTEP )
			NDebugOverlay::Box( stepTrace.endpos, WorldAlignMins(), WorldAlignMaxs(), 96, 96, 96, 0, 5 );
#endif // !(CLIENT_DLL) && defined( UNIT_DEBUGSTEP )

			stepStart = stepTrace.endpos;
		}
		else
			stepStart.z += args.stepHeight;

		// Now move forward
		stepEnd.z = stepStart.z;

		TraceUnitBBox( stepStart, stepEnd, unitsolidmask, m_pOuter->GetCollisionGroup(), stepTrace );
		bool bRejectStep = false;

		// Ok, raising it didn't work; we're obstructed
		if (stepTrace.startsolid || stepTrace.fraction <= 0.01 )
		{
			// If started in solid, and never escaped from solid, bail
			if ( trace.startsolid )
			{
				pResult->fStartSolid = true;
				pResult->pBlocker = trace.m_pEnt;
				pResult->hitPos = trace.endpos;
				pResult->hitNormal = trace.plane.normal;
				return false;
			}

			bRejectStep = true;
		}
		else
		{
#if !defined(CLIENT_DLL) && defined( UNIT_DEBUGSTEP )
			NDebugOverlay::Box( stepTrace.endpos, WorldAlignMins(), WorldAlignMaxs(), 128, 128, 128, 0, 5 );
#endif // !(CLIENT_DLL) && defined( UNIT_DEBUGSTEP )

			// If didn't step forward enough to qualify as a step, try as if stepped forward to
			// confirm there's potentially enough space to "land"
			float landingDistSq = ( stepEnd.AsVector2D() - stepStart.AsVector2D() ).LengthSqr();
			float requiredLandingDistSq = args.minStepLanding*args.minStepLanding;
			if ( landingDistSq < requiredLandingDistSq )
			{
				trace_t landingTrace;
				Vector stepEndWithLanding;

				VectorMA( stepStart, args.minStepLanding, args.vecStepDir, stepEndWithLanding );
				TraceUnitBBox( stepStart, stepEndWithLanding, unitsolidmask, m_pOuter->GetCollisionGroup(), landingTrace );
				if ( landingTrace.fraction < 1 )
				{
#if !defined(CLIENT_DLL) && defined( UNIT_DEBUGSTEP )
					NDebugOverlay::Box( landingTrace.endpos, WorldAlignMins() + Vector(0, 0, 0.1), WorldAlignMaxs() + Vector(0, 0, 0.1), 255, 0, 0, 0, 5 );
#endif // !(CLIENT_DLL) && defined( UNIT_DEBUGSTEP )

					bRejectStep = true;
					if ( landingTrace.m_pEnt ) {
						pResult->pBlocker = landingTrace.m_pEnt;
						pResult->hitPos = landingTrace.endpos;
					}
				}
			}
			else if ( ( stepTrace.endpos.AsVector2D() - stepStart.AsVector2D() ).LengthSqr() < requiredLandingDistSq )
			{
#if !defined(CLIENT_DLL) && defined( UNIT_DEBUGSTEP )
				NDebugOverlay::Box( stepTrace.endpos, WorldAlignMins() + Vector(0, 0, 0.1), WorldAlignMaxs() + Vector(0, 0, 0.1), 255, 0, 0, 0, 5 );
#endif // !(CLIENT_DLL) && defined( UNIT_DEBUGSTEP )

				bRejectStep = true;
			}
		}

		// If trace.fraction == 0, we fall through and check the position
		// we moved up to for suitability. This allows for sub-step
		// traces if the position ends up being suitable
		if ( !bRejectStep )
			trace = stepTrace;

		if ( trace.fraction < 1.0 )
		{
			if ( !pResult->pBlocker )
				pResult->pBlocker = trace.m_pEnt;
			pResult->hitPos = trace.endpos;
			pResult->hitNormal = trace.plane.normal;
		}

		stepEnd = trace.endpos;
	}

	// seems okay, now find the ground
	// The ground is only valid if it's within a step height of the original position
	Assert( VectorsAreEqual( trace.endpos, stepEnd, 1e-3 ) );
	stepStart = stepEnd; 
	stepEnd.z = args.vecStart.z - args.stepHeight * args.stepDownMultiplier /*- UNIT_MOVE_HEIGHT_EPSILON*/;

	TraceUnitBBox( stepStart, stepEnd, unitsolidmask, m_pOuter->GetCollisionGroup(), trace );

	// Return a point that is *on the ground*
	// We'll raise it by an epsilon in check step again
	pResult->endPoint = trace.endpos;
	//pResult->endPoint.z += UNIT_MOVE_HEIGHT_EPSILON; // always safe because always stepped down at least by epsilon

#if !defined(CLIENT_DLL) && defined( UNIT_DEBUGSTEP )
		NDebugOverlay::Cross3D( trace.endpos, 32, 0, 255, 0, true, 5 );
#endif // !(CLIENT_DLL) && defined( UNIT_DEBUGSTEP )

	return ( pResult->pBlocker == NULL ); // totally clear if pBlocker is NULL, partial blockage otherwise
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnitBaseLocomotion::Friction( void )
{
	float	speed, newspeed, control;
	float	friction;
	float	drop;
	
	// If we are in water jump cycle, don't apply friction
	//if (m_pOuter->m_flWaterJumpTime)
	//	return;

	// Calculate speed
	speed = VectorLength( mv->velocity );
	
	// If too slow, return
	if (speed < 0.1f)
	{
		return;
	}

	drop = 0;

	// apply ground friction
	if (m_pOuter->GetGroundEntity() != NULL)  // On an entity that is the ground
	{
		friction = worldfriction * surfacefriction;

		// Bleed off some speed, but if we have less than the bleed
		//  threshold, bleed the threshold amount.
		control = (speed < stopspeed) ? stopspeed : speed;

		// Add the amount to the drop amount.
		drop += control*friction*mv->interval;
	}

	// scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0)
		newspeed = 0;

	if ( newspeed != speed )
	{
		// Determine proportion of old speed we are using.
		newspeed /= speed;
		// Adjust velocity according to proportion.
		VectorScale( mv->velocity, newspeed, mv->velocity );
	}

 	mv->outwishvel -= (1.f-newspeed) * mv->velocity;

#ifdef DEBUG_MOVEMENT_VELOCITY
	if( !bMovement && bVelocity )
	{
		Msg("Friction. Vel: %f. Drop: %f. Speed: %f. NewSpeed: %f. Friction: %f. World friction: %f. Surface friction: %f. Stopspeed: %f\n",
			VectorLength( mv->velocity ), drop, speed, newspeed, friction, worldfriction, surfacefriction, stopspeed);
	}
#endif // DEBUG_MOVEMENT_VELOCITY
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnitBaseLocomotion::StartGravity( void )
{
	float ent_gravity;
	
	if (m_pOuter->GetGravity())
		ent_gravity = m_pOuter->GetGravity();
	else
		ent_gravity = 1.0;

	// Add gravity so they'll be in the correct position during movement
	// yes, this 0.5 looks wrong, but it's not.  
	mv->velocity[2] -= (ent_gravity * sv_gravity.GetFloat() * 0.5 * mv->interval );
	mv->velocity[2] += m_pOuter->GetBaseVelocity()[2] * mv->interval;

	Vector temp = m_pOuter->GetBaseVelocity();
	temp[ 2 ] = 0;
	m_pOuter->SetBaseVelocity( temp );

	//CheckVelocity();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnitBaseLocomotion::FinishGravity( void )
{
	float ent_gravity;

	//if ( m_pOuter->m_flWaterJumpTime )
	//	return;

	if ( m_pOuter->GetGravity() )
		ent_gravity = m_pOuter->GetGravity();
	else
		ent_gravity = 1.0;

	// Get the correct velocity for the end of the dt 
  	mv->velocity[2] -= (ent_gravity * sv_gravity.GetFloat() * mv->interval * 0.5);

	//CheckVelocity();
}

//-----------------------------------------------------------------------------
// Purpose: The basic solid body movement attempt/clip that slides along multiple planes
// Input  : time - Amount of time to try moving for
//			*steptrace - if not NULL, the trace results of any vertical wall hit will be stored
// Output : int - the clipflags if the velocity was modified (hit something solid)
//   1 = floor
//   2 = wall / step
//   4 = dead stop
//-----------------------------------------------------------------------------
int UnitBaseLocomotion::UnitTryMove( trace_t *steptrace )
{
	VPROF("UnitBaseLocomotion::PhysicsTryMove");

	int			bumpcount, numbumps;
	Vector		dir;
	float		d;
	int			numplanes;
	Vector		planes[MAX_CLIP_PLANES];
	Vector		primal_velocity, original_velocity, new_velocity;
	int			i, j;
	trace_t		trace;
	Vector		end;
	float		time_left;
	int			blocked;
	
	unsigned int mask = unitsolidmask;

	new_velocity.Init();

	numbumps = 4;

	Vector vecAbsVelocity = mv->velocity;

	blocked = 0;
	VectorCopy (vecAbsVelocity, original_velocity);
	VectorCopy (vecAbsVelocity, primal_velocity);
	numplanes = 0;
	
	time_left = mv->interval;

	mv->blocker_hitpos = vec3_origin;
	mv->m_hBlocker = NULL;

	for (bumpcount=0 ; bumpcount<numbumps ; bumpcount++)
	{
		if (vecAbsVelocity == vec3_origin)
			break;

		VectorMA( mv->origin, time_left, vecAbsVelocity, end );

		TraceUnitBBox(  mv->origin, end, mask, m_pOuter->GetCollisionGroup(), trace );

		if (trace.startsolid)
		{	// entity is trapped in another solid
			mv->velocity = vec3_origin;
			return 4;
		}

		if (trace.fraction > 0)
		{	// actually covered some distance
			mv->origin = trace.endpos;
			VectorCopy (vecAbsVelocity, original_velocity);
			numplanes = 0;
		}

		if (trace.fraction == 1)
			 break;		// moved the entire distance

		if (!trace.m_pEnt)
		{
			mv->velocity = vecAbsVelocity;
			Warning( "PhysicsTryMove: !trace.u.ent" );
			Assert(0);
			return 4;
		}

		// Save current blocker + hitpos
		mv->m_hBlocker = trace.m_pEnt; 
		mv->blocker_hitpos = trace.endpos; 
		mv->blocker_dir = vecAbsVelocity.Normalized();
		/*if (trace.plane.normal[2] > 0.7)
		{
			blocked |= 1;		// floor
			if ( trace.m_pEnt->IsStandable() )
			{
				// keep track of time when changing ground entity
				if (GetGroundEntity() != trace.m_pEnt)
				{
					m_pOuter->SetGroundChangeTime( gpGlobals->curtime + (mv->interval - (1 - trace.fraction) * time_left) );
				}

				SetGroundEntity( trace.m_pEnt );
			}
		}*/
		if (!trace.plane.normal[2])
		{
			blocked |= 2;		// step
			if (steptrace)
				*steptrace = trace;	// save for unit extrafriction
		}

		// The code below quickly makes everything runs super slow
		// Some problem with the touch stuff...
#ifndef CLIENT_DLL
		// run the impact function
		m_pOuter->PhysicsImpact( trace.m_pEnt, trace );
		// Removed by the impact function
		if ( m_pOuter->IsMarkedForDeletion() || m_pOuter->IsEdictFree() )
			break;	
#endif // CLIENT_DLL

		time_left -= time_left * trace.fraction;
		
		// clipped to another plane
		if (numplanes >= MAX_CLIP_PLANES)
		{	// this shouldn't really happen
			mv->velocity = vec3_origin;
			return blocked;
		}

		VectorCopy (trace.plane.normal, planes[numplanes]);
		numplanes++;

		// modify original_velocity so it parallels all of the clip planes
		if ( m_pOuter->GetMoveType() == MOVETYPE_WALK && (!(GetFlags() & FL_ONGROUND) || m_pOuter->GetFriction()!=1) )	// relfect m_pOuter velocity
		{
			for ( i = 0; i < numplanes; i++ )
			{
				if ( planes[i][2] > 0.7  )
				{// floor or slope
					ClipVelocity( original_velocity, planes[i], new_velocity, 1 );
					VectorCopy( new_velocity, original_velocity );
				}
				else
				{
					ClipVelocity( original_velocity, planes[i], new_velocity, 1.0 + sv_bounce.GetFloat() * (1-m_pOuter->GetFriction()) );
				}
			}

			VectorCopy( new_velocity, vecAbsVelocity );
			VectorCopy( new_velocity, original_velocity );
		}
		else
		{
			for (i=0 ; i<numplanes ; i++)
			{
				ClipVelocity (original_velocity, planes[i], new_velocity, 1);
				for (j=0 ; j<numplanes ; j++)
					if (j != i)
					{
						if (DotProduct (new_velocity, planes[j]) < 0)
							break;	// not ok
					}
				if (j == numplanes)
					break;
			}
			
			if (i != numplanes)
			{	
				// go along this plane
				VectorCopy (new_velocity, vecAbsVelocity);
			}
			else
			{	
				// go along the crease
				if (numplanes != 2)
				{
	//				Msg( "clip velocity, numplanes == %i\n",numplanes);
					mv->velocity = vecAbsVelocity;
					return blocked;
				}
				CrossProduct (planes[0], planes[1], dir);
				d = DotProduct (dir, vecAbsVelocity);
				VectorScale (dir, d, vecAbsVelocity);
			}

			//
			// if original velocity is against the original velocity, stop dead
			// to avoid tiny oscillations in sloping corners
			//
			if (DotProduct (vecAbsVelocity, primal_velocity) <= 0)
			{
				mv->velocity = vec3_origin;
				return blocked;
			}
		}
	}

	mv->velocity = vecAbsVelocity;
	return blocked;
}

//-----------------------------------------------------------------------------
// Purpose: Does the basic move attempting to climb up step heights.  It uses
//          the mv->origin and mv->velocity.  It returns a new
//          new mv->origin, mv->velocity, and mv->m_outStepHeight.
//-----------------------------------------------------------------------------
void UnitBaseLocomotion::StepMove( Vector &vecDestination, trace_t &trace )
{
	Vector vecEndPos;
	VectorCopy( vecDestination, vecEndPos );

	// Try sliding forward both on ground and up 16 pixels
	//  take the move that goes farthest
	Vector vecPos, vecVel;
	VectorCopy( mv->origin, vecPos );
	VectorCopy( mv->velocity, vecVel );

	// Slide move down.
	TryUnitMove( &vecEndPos, &trace );
	
	// Down results.
	Vector vecDownPos, vecDownVel;
	VectorCopy( mv->origin, vecDownPos );
	VectorCopy( mv->velocity, vecDownVel );
	
	// Reset original values.
	mv->origin = vecPos;
	VectorCopy( vecVel, mv->velocity );
	
	// Move up a stair height.
	VectorCopy( mv->origin, vecEndPos );
	//if ( m_pOuter->m_Local.m_bAllowAutoMovement )
	{
		vecEndPos.z += stepsize + DIST_EPSILON;
	}
	
	TraceUnitBBox( mv->origin, vecEndPos, unitsolidmask, m_pOuter->GetCollisionGroup(), trace );
	if ( !trace.startsolid && !trace.allsolid )
	{
		mv->origin = trace.endpos;
	}
	
	// Slide move up.
	TryUnitMove();
	
	// Move down a stair (attempt to).
	VectorCopy( mv->origin, vecEndPos );
	//if ( m_pOuter->m_Local.m_bAllowAutoMovement )
	{
		vecEndPos.z -= stepsize + DIST_EPSILON;
	}
		
	TraceUnitBBox( mv->origin, vecEndPos, unitsolidmask, m_pOuter->GetCollisionGroup(), trace );
	
	// If we are not on the ground any more then use the original movement attempt.
	if ( trace.plane.normal[2] < 0.7 )
	{
		mv->origin = vecDownPos;
		VectorCopy( vecDownVel, mv->velocity );
		float flStepDist = mv->origin.z - vecPos.z;
		if ( flStepDist > 0.0f )
		{
			mv->outstepheight += flStepDist;
		}
		return;
	}
	
	// If the trace ended up in empty space, copy the end over to the origin.
	if ( !trace.startsolid && !trace.allsolid )
	{
		mv->origin = trace.endpos;
	}
	
	// Copy this origin to up.
	Vector vecUpPos;
	VectorCopy( mv->origin, vecUpPos );
	
	// decide which one went farther
	float flDownDist = ( vecDownPos.x - vecPos.x ) * ( vecDownPos.x - vecPos.x ) + ( vecDownPos.y - vecPos.y ) * ( vecDownPos.y - vecPos.y );
	float flUpDist = ( vecUpPos.x - vecPos.x ) * ( vecUpPos.x - vecPos.x ) + ( vecUpPos.y - vecPos.y ) * ( vecUpPos.y - vecPos.y );
	if ( flDownDist > flUpDist )
	{
		mv->origin = vecDownPos;
		VectorCopy( vecDownVel, mv->velocity );
	}
	else 
	{
		// copy z value from slide move
		mv->velocity.z = vecDownVel.z;
	}
	
	float flStepDist = mv->origin.z - vecPos.z;
	if ( flStepDist > 0 )
	{
		mv->outstepheight += flStepDist;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int UnitBaseLocomotion::TryUnitMove( Vector *pFirstDest, trace_t *pFirstTrace )
{
	int			bumpcount, numbumps;
	Vector		dir;
	float		d;
	int			numplanes;
	Vector		planes[MAX_CLIP_PLANES];
	Vector		primal_velocity, original_velocity;
	Vector      new_velocity;
	int			i, j;
	trace_t	pm;
	Vector		end;
	float		time_left, allFraction;
	int			blocked;		
	
	numbumps  = 4;           // Bump up to four times
	
	blocked   = 0;           // Assume not blocked
	numplanes = 0;           //  and not sliding along any planes

	VectorCopy (mv->velocity, original_velocity);  // Store original velocity
	VectorCopy (mv->velocity, primal_velocity);
	
	allFraction = 0;
	time_left = mv->interval;   // Total time for this movement operation.

	new_velocity.Init();

	for (bumpcount=0 ; bumpcount < numbumps; bumpcount++)
	{
		if ( mv->velocity.Length() == 0.0 )
			break;

		// Assume we can move all the way from the current origin to the
		//  end point.
		VectorMA( mv->origin, time_left, mv->velocity, end );

		// See if we can make it from origin to end point.
		if ( /*g_bMovementOptimizations*/ 1 )
		{
			// If their velocity Z is 0, then we can avoid an extra trace here during WalkMove.
			if ( pFirstDest && end == *pFirstDest )
				pm = *pFirstTrace;
			else
			{
#if defined( PLAYER_GETTING_STUCK_TESTING )
				trace_t foo;
				Tracem_pOuterBBox( mv->origin, mv->origin, m_pOuterSolidMask(), m_pOuter->GetCollisionGroup(), foo );
				if ( foo.startsolid || foo.fraction != 1.0f )
				{
					Msg( "bah\n" );
				}
#endif
				TraceUnitBBox( mv->origin, end, unitsolidmask, m_pOuter->GetCollisionGroup(), pm );
			}
		}
		else
		{
			TraceUnitBBox( mv->origin, end, unitsolidmask, m_pOuter->GetCollisionGroup(), pm );
		}

		allFraction += pm.fraction;

		// If we started in a solid object, or we were in solid space
		//  the whole way, zero out our velocity and return that we
		//  are blocked by floor and wall.
		if (pm.allsolid)
		{	
			// entity is trapped in another solid
			VectorCopy (vec3_origin, mv->velocity);
			return 4;
		}

		// If we moved some portion of the total distance, then
		//  copy the end position into the pmove.origin and 
		//  zero the plane counter.
		if( pm.fraction > 0 )
		{	
			if ( numbumps > 0 && pm.fraction == 1 )
			{
				// There's a precision issue with terrain tracing that can cause a swept box to successfully trace
				// when the end position is stuck in the triangle.  Re-run the test with an uswept box to catch that
				// case until the bug is fixed.
				// If we detect getting stuck, don't allow the movement
				trace_t stuck;
				TraceUnitBBox( pm.endpos, pm.endpos, unitsolidmask, m_pOuter->GetCollisionGroup(), stuck );
				if ( stuck.startsolid || stuck.fraction != 1.0f )
				{
					//Msg( "m_pOuter will become stuck!!!\n" );
					VectorCopy (vec3_origin, mv->velocity);
					break;
				}
			}

#if defined( PLAYER_GETTING_STUCK_TESTING )
			trace_t foo;
			Tracem_pOuterBBox( pm.endpos, pm.endpos, m_pOuterSolidMask(), m_pOuter->GetCollisionGroup(), foo );
			if ( foo.startsolid || foo.fraction != 1.0f )
			{
				Msg( "m_pOuter will become stuck!!!\n" );
			}
#endif
			// actually covered some distance
			mv->origin = pm.endpos;
			VectorCopy (mv->velocity, original_velocity);
			numplanes = 0;
		}

		// If we covered the entire distance, we are done
		//  and can return.
		if (pm.fraction == 1)
		{
			 break;		// moved the entire distance
		}

		// Save entity that blocked us (since fraction was < 1.0)
		//  for contact
		// Add it if it's not already in the list!!!
		//MoveHelper( )->AddToTouched( pm, mv->velocity );

		// If the plane we hit has a high z component in the normal, then
		//  it's probably a floor
		if (pm.plane.normal[2] > 0.7)
		{
			blocked |= 1;		// floor
		}
		// If the plane has a zero z component in the normal, then it's a 
		//  step or wall
		if (!pm.plane.normal[2])
		{
			blocked |= 2;		// step / wall
		}

		// Reduce amount of m_flFrameTime left by total time left * fraction
		//  that we covered.
		time_left -= time_left * pm.fraction;

		// Did we run out of planes to clip against?
		if (numplanes >= MAX_CLIP_PLANES)
		{	
			// this shouldn't really happen
			//  Stop our movement if so.
			VectorCopy (vec3_origin, mv->velocity);
			//Con_DPrintf("Too many planes 4\n");

			break;
		}

		// Set up next clipping plane
		VectorCopy (pm.plane.normal, planes[numplanes]);
		numplanes++;

		// modify original_velocity so it parallels all of the clip planes
		//

		// reflect m_pOuter velocity 
		// Only give this a try for first impact plane because you can get yourself stuck in an acute corner by jumping in place
		//  and pressing forward and nobody was really using this bounce/reflection feature anyway...
		if ( numplanes == 1 &&
			m_pOuter->GetMoveType() == MOVETYPE_WALK &&
			m_pOuter->GetGroundEntity() == NULL )	
		{
			for ( i = 0; i < numplanes; i++ )
			{
				if ( planes[i][2] > 0.7  )
				{
					// floor or slope
					ClipVelocity( original_velocity, planes[i], new_velocity, 1 );
					VectorCopy( new_velocity, original_velocity );
				}
				else
				{
					ClipVelocity( original_velocity, planes[i], new_velocity, 1.0 + sv_bounce.GetFloat() * (1 - surfacefriction) );
				}
			}

			VectorCopy( new_velocity, mv->velocity );
			VectorCopy( new_velocity, original_velocity );
		}
		else
		{
			for (i=0 ; i < numplanes ; i++)
			{
				ClipVelocity (
					original_velocity,
					planes[i],
					mv->velocity,
					1);

				for (j=0 ; j<numplanes ; j++)
					if (j != i)
					{
						// Are we now moving against this plane?
						if (mv->velocity.Dot(planes[j]) < 0)
							break;	// not ok
					}
				if (j == numplanes)  // Didn't have to clip, so we're ok
					break;
			}
			
			// Did we go all the way through plane set
			if (i != numplanes)
			{	// go along this plane
				// pmove.velocity is set in clipping call, no need to set again.
				;  
			}
			else
			{	// go along the crease
				if (numplanes != 2)
				{
					VectorCopy (vec3_origin, mv->velocity);
					break;
				}
				CrossProduct (planes[0], planes[1], dir);
				dir.NormalizeInPlace();
				d = dir.Dot(mv->velocity);
				VectorScale (dir, d, mv->velocity );
			}

			//
			// if original velocity is against the original velocity, stop dead
			// to avoid tiny occilations in sloping corners
			//
			d = mv->velocity.Dot(primal_velocity);
			if (d <= 0)
			{
				//Con_DPrintf("Back\n");
				VectorCopy (vec3_origin, mv->velocity);
				break;
			}
		}
	}

	if ( allFraction == 0 )
	{
		VectorCopy (vec3_origin, mv->velocity);
	}

	/*
	// Check if they slammed into a wall
	float fSlamVol = 0.0f;

	float fLateralStoppingAmount = primal_velocity.Length2D() - mv->velocity.Length2D();
	if ( fLateralStoppingAmount > m_pOuter_MAX_SAFE_FALL_SPEED * 2.0f )
	{
		fSlamVol = 1.0f;
	}
	else if ( fLateralStoppingAmount > m_pOuter_MAX_SAFE_FALL_SPEED )
	{
		fSlamVol = 0.85f;
	}

	m_pOuterRoughLandingEffects( fSlamVol );
	*/

	return blocked;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : in - 
//			normal - 
//			out - 
//			overbounce - 
// Output : int
//-----------------------------------------------------------------------------
int UnitBaseLocomotion::ClipVelocity( Vector& in, Vector& normal, Vector& out, float overbounce )
{
	float	backoff;
	float	change;
	float angle;
	int		i, blocked;
	
	angle = normal[ 2 ];

	blocked = 0x00;         // Assume unblocked.
	if (angle > 0)			// If the plane that is blocking us has a positive z component, then assume it's a floor.
		blocked |= 0x01;	// 
	if (!angle)				// If the plane has no Z, it is vertical (wall/step)
		blocked |= 0x02;	// 
	

	// Determine how far along plane to slide based on incoming direction.
	backoff = DotProduct (in, normal) * overbounce;

	for (i=0 ; i<3 ; i++)
	{
		change = normal[i]*backoff;
		out[i] = in[i] - change; 
	}
	
	// iterate once to make sure we aren't still moving through the plane
	float adjust = DotProduct( out, normal );
	if( adjust < 0.0f )
	{
		out -= ( normal * adjust );
//		Msg( "Adjustment = %lf\n", adjust );
	}

	// Return blocking flags.
	return blocked;
}

void UnitBaseLocomotion::SetupMovementBounds( UnitBaseMoveCommand &mv )
{
	m_vecMins = GetOuter()->CollisionProp()->OBBMins();
	m_vecMaxs = GetOuter()->CollisionProp()->OBBMaxs();

	if ( m_pTraceListData )
	{
		m_pTraceListData->Reset();
	}
	else
	{
		m_pTraceListData = enginetrace->AllocTraceListData();
	}

	Vector moveMins, moveMaxs;
	ClearBounds( moveMins, moveMaxs );
	Vector start = mv.origin;
	float radius = ((mv.velocity.Length() + mv.maxspeed) * mv.interval) + 1.0f;

	// NOTE: assumes the unducked bbox encloses the ducked bbox
	Vector boxMins = m_vecMins;
	Vector boxMaxs = m_vecMaxs;

	// bloat by traveling the max velocity in all directions, plus the stepsize up/down
	Vector bloat;
	bloat.Init(radius, radius, radius);
	bloat.z += MAX(stepsize, (stepsize*(mv.interval*mv.maxspeed/48.0)));
	AddPointToBounds( start + boxMaxs + bloat, moveMins, moveMaxs );
	AddPointToBounds( start + boxMins - bloat, moveMins, moveMaxs );
	// now build an optimized trace within these bounds
	enginetrace->SetupLeafAndEntityListBox( moveMins, moveMaxs, m_pTraceListData );
}