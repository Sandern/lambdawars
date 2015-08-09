//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: Locomotion for vphysics based rollermine. Makes the unit roll in the
//			path direction.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "unit_vphysicslocomotion.h"
#include "movevars_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Movement class
//-----------------------------------------------------------------------------
#ifdef ENABLE_PYTHON
UnitVPhysicsLocomotion::UnitVPhysicsLocomotion( boost::python::object outer ) : UnitBaseLocomotion(outer), m_pMotionController(NULL)
{
	m_vecAngular.Init();
}
#endif // ENABLE_PYTHON

UnitVPhysicsLocomotion::~UnitVPhysicsLocomotion()
{
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void UnitVPhysicsLocomotion::SetupMove( UnitBaseMoveCommand &mv )
{
	mv.blockers.RemoveAll();

	IPhysicsObject *pPhysObj = GetOuter()->VPhysicsGetObject();
	if( !pPhysObj )
	{
		Warning( "UnitVPhysicsLocomotion: No VPhysics object for entity #%d\n", GetOuter()->entindex() );
		return;
	}

#ifdef CLIENT_DLL
	mv.origin = m_pOuter->GetNetworkOrigin();
	mv.viewangles = mv.idealviewangles;
#else
	mv.viewangles = mv.idealviewangles;
	mv.origin = m_pOuter->GetAbsOrigin();
#endif // CLIENT_DLL

	pPhysObj->GetVelocity( &(mv.velocity), NULL );

	// Clamp maxspeed
	mv.maxspeed = Min(mv.maxspeed, 5000.0f);

	SetupMovementBounds( mv );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void UnitVPhysicsLocomotion::FinishMove( UnitBaseMoveCommand &mv )
{
	m_pOuter->PhysicsTouchTriggers();

	//m_pOuter->SetAbsVelocity( mv.velocity );

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
// Main move function
//-----------------------------------------------------------------------------
void UnitVPhysicsLocomotion::Move( float interval, UnitBaseMoveCommand &move_command )
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
// If I were to stop moving, how much distance would I walk before I'm halted?
//-----------------------------------------------------------------------------
float UnitVPhysicsLocomotion::GetStopDistance()
{

	float	speed, newspeed, control;
	float	friction;
	float	drop;
	float	distance;
	int		i;
	
	// Calculate speed
	if( mv->velocity.IsValid() )
		speed = mv->velocity.Length();
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
			drop += control * friction * mv->interval;
		}
		else
		{
			// apply friction
			friction = sv_friction.GetFloat() * surfacefriction;

			// Bleed off some speed, but if we have less than the bleed
			//  threshold, bleed the threshold amount.
			control = (speed < sv_stopspeed.GetFloat()) ? sv_stopspeed.GetFloat() : speed;

			// Add the amount to the drop amount.
			drop += control * friction * mv->interval;
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

	return distance;
}


//-----------------------------------------------------------------------------
// Walking
//-----------------------------------------------------------------------------
void UnitVPhysicsLocomotion::VPhysicsMove( void )
{
	int i;

	Vector wishvel;
	float fmove, smove, umove;
	Vector wishdir;
	float wishspeed;

	Vector dest;
	Vector forward, right, up;

	QAngle viewangles = mv->viewangles;
	viewangles.x = viewangles.z = 0;
	AngleVectors (viewangles, &forward, &right, &up);  // Determine movement angles

	CHandle< CBaseEntity > oldground;
	oldground = GetOuter()->GetGroundEntity();

	// Copy movement amounts
	fmove = mv->forwardmove;
	smove = mv->sidemove;
	umove = mv->upmove;

	for ( i = 0 ; i < 2 ; i++ )       // Determine x and y parts of velocity
		wishvel[i] = forward[i]*fmove + right[i]*smove + up[i]*umove;

	VectorCopy (wishvel, wishdir);   // Determine maginitude of speed of move

	//Vector vTransformedVelocity;
	//VectorRotate( wishvel, /*GetOuter()->GetAbsAngles()*/ QAngle(-45, 0, 0), wishdir );

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

#ifdef CLIENT_DLL
static AngularImpulse WorldToLocalRotation( const VMatrix &localToWorld, const Vector &worldAxis, float rotation )
{
	// fix axes of rotation to match axes of vector
	Vector rot = worldAxis * rotation;
	// since the matrix maps local to world, do a transpose rotation to get world to local
	AngularImpulse ang = localToWorld.VMul3x3Transpose( rot );

	return ang;
}
#endif // CLIENT_DLL

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

	Vector stepEnd;
	int i;

	// Clear current blocker
	ClearBlockers();

	// Test our start position.
	// Set blocker to ignore if allowed.
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
	}

	// When we are in a solid for whatever reason and can't ignore it, try to unstuck
	if( trace.startsolid && !(trace.m_pEnt && trace.m_pEnt->AllowNavIgnore()) )
	{
		DoUnstuck();
	}

	if( !m_pMotionController )
	{
		Vector vecRight;
		AngleVectors( mv->viewangles, NULL, &vecRight, NULL );

		AngularImpulse impulse = WorldToLocalRotation( SetupMatrixAngles(GetLocalAngles()), vecRight, -mv->velocity.Length() );

		//m_vecAngular = m_vecAngular + (impulse - m_vecAngular) * mv->interval;
		//m_vecAngular = impulse;
		m_vecAngular = 0.2f * m_vecAngular + 0.8f * impulse;

		pPhysObj->SetVelocity( NULL, &m_vecAngular );
	}

#if 0
	Vector curVel, curAngVel;
	pPhysObj->GetVelocity( &curVel, &curAngVel );
	engine->Con_NPrintf( 0, "Velocity: %f %f %f (%f)", mv->velocity.x, mv->velocity.y, mv->velocity.z, mv->velocity.Length() );
	engine->Con_NPrintf( 2, "Cur Velocity: %f %f %f (%f)", curVel.x, curVel.y, curVel.z, curVel.Length() );
	engine->Con_NPrintf( 3, "Cur Ang Velocity: %f %f %f (%f)", curAngVel.x, curAngVel.y, curAngVel.z, curAngVel.Length() );
#endif // 0

	// Clear nav ignored entities	
	for ( i = 0; i < ignoredEntities.Count(); i++ )
	{
		ignoredEntities[i]->ClearNavIgnore();
	}
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void UnitVPhysicsLocomotion::CreateMotionController()
{
	DestroyMotionController();

	IPhysicsObject *pPhysObj = m_pOuter->VPhysicsGetObject();
	if( !pPhysObj )
	{
		return;
	}

	m_pMotionController = physenv->CreateMotionController( this );
	m_pMotionController->AttachObject( pPhysObj, true );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void UnitVPhysicsLocomotion::DestroyMotionController()
{
	if ( m_pMotionController != NULL )
	{
		physenv->DestroyMotionController( m_pMotionController );
		m_pMotionController = NULL;
	}
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
IMotionEvent::simresult_e UnitVPhysicsLocomotion::Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular )
{
	m_vecLinear.Init(0, 0, 0);

	if( mv )
	{
		Vector vecRight;
		AngleVectors( mv->viewangles, NULL, &vecRight, NULL );

		AngularImpulse impulse = WorldToLocalRotation( SetupMatrixAngles(GetLocalAngles()), vecRight, -mv->velocity.Length() );

		m_vecAngular = m_vecAngular + (impulse - m_vecAngular) * deltaTime;
	}

	linear = m_vecLinear;
	angular = m_vecAngular;

	return SIM_LOCAL_ACCELERATION;
}

