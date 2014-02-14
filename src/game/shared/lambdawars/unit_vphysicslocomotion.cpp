//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "unit_vphysicslocomotion.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Movement class
//-----------------------------------------------------------------------------
#ifdef ENABLE_PYTHON
UnitVPhysicsLocomotion::UnitVPhysicsLocomotion( boost::python::object outer ) : UnitBaseLocomotion(outer)
{

}
#endif // ENABLE_PYTHON

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void UnitVPhysicsLocomotion::FinishMove( UnitBaseMoveCommand &mv )
{
	m_pOuter->PhysicsTouchTriggers();

#ifdef ENABLE_PYTHON
	// For Python: keep list of blockers
	mv.pyblockers = boost::python::list();
	for( int i = 0; i < mv.blockers.Count(); i++ )
	{
		if( !mv.blockers[i].blocker )
			continue;
 		mv.pyblockers.append( mv.blockers[i].blocker->GetPyHandle() );
	}
#endif // ENABLE_PYTHON
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnitVPhysicsLocomotion::FullWalkMove( )
{
	StartGravity();

	CategorizePosition();

	// Clear ground entity, so we will do an air move.
	if( mv->jump )
		HandleJump();

	VPhysicsMove();

	FinishGravity();

	// If we are on ground, no downward velocity.
	if( GetGroundEntity() != NULL )
		mv->velocity.z = 0.0f;
}

//-----------------------------------------------------------------------------
// Walking
//-----------------------------------------------------------------------------
void UnitVPhysicsLocomotion::VPhysicsMove( void )
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
	Accelerate ( wishdir, wishspeed, acceleration );

	// first try just moving to the destination	
	//dest[0] = mv->origin[0] + mv->velocity[0]*mv->interval;
	//dest[1] = mv->origin[1] + mv->velocity[1]*mv->interval;
	//dest[2] = mv->origin[2];

	// Add in any base velocity to the current velocity.
	//VectorAdd (mv->velocity, m_pOuter->GetBaseVelocity(), mv->velocity );

	// 2d movement (xy)
	//GroundMove(mv->origin, dest, unitsolidmask, 0);
	VPhysicsMoveStep();

	// Now pull the base velocity back out.   Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
	//VectorSubtract( mv->velocity, m_pOuter->GetBaseVelocity(), mv->velocity );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void UnitVPhysicsLocomotion::VPhysicsMoveStep()
{
	IPhysicsObject *pPhysObj = GetOuter()->VPhysicsGetObject();
	if( !pPhysObj )
	{
		Warning( "UnitVPhysicsLocomotion: No VPhysics object for entity #%d\n", GetOuter()->entindex() );
		return;
	}

	CUtlVector<CBaseEntity *> ignoredEntities;
	trace_t trace;
	Vector stepEnd;
	float fIntervalStepSize;
	int i;

	// Clear current blocker
	ClearBlockers();

	// Calculate the max stepsize for this interval
	// Assume we can move up/down stepsize per 48.0
	fIntervalStepSize = Max<float>( stepsize, (stepsize * ( mv->interval*mv->maxspeed/48.0 ) ) );

	// Test our start position.
	// Set blocker to ignore if allowed.
	stepEnd = mv->origin;
	stepEnd.z = mv->origin.z + 1.0f;
	for( i = 0; i < 8; i++ )
	{
		TraceUnitBBox( mv->origin, stepEnd, unitsolidmask, m_pOuter->GetCollisionGroup(), trace );
		if( !trace.startsolid || !trace.m_pEnt || 
			(!trace.m_pEnt->AllowNavIgnore() && trace.m_pEnt->GetMoveType() != MOVETYPE_VPHYSICS) )
			break;
		ignoredEntities.AddToTail( trace.m_pEnt );
		trace.m_pEnt->SetNavIgnore();
	}

	Vector curVel, curAngVel;
	pPhysObj->GetVelocity( &curVel, &curAngVel );

	Vector vAddVel = mv->velocity;
	VectorNormalize( vAddVel );
	vAddVel *= mv->interval*mv->maxspeed;
	
	pPhysObj->AddVelocity( &vAddVel, NULL );
	//pPhysObj->SetVelocity( &(mv->velocity), NULL );

	// Clear nav ignored entities	
	for ( i = 0; i < ignoredEntities.Count(); i++ )
	{
		ignoredEntities[i]->ClearNavIgnore();
	}
}