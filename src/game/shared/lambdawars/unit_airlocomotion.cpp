//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "unit_airlocomotion.h"
#include "movevars_shared.h"
#include "coordsize.h"

#ifdef ENABLE_PYTHON
#include "srcpy.h"
#endif // ENABLE_PYTHON

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
#ifdef ENABLE_PYTHON
UnitBaseAirLocomotion::UnitBaseAirLocomotion( boost::python::object outer )
	:UnitBaseLocomotion(outer)
{
	m_fCurrentHeight = 0;
	m_fDesiredHeight = 450.0f;
	m_fMaxHeight = 0;
	m_fFlyNoiseZ = 0.0f;
	m_fFlyNoiseRate = 0.0f;
	m_fFlyCurNoise = 0.0f;
	m_bFlyNoiseUp = true;
}

boost::python::object UnitBaseAirLocomotion::CreateMoveCommand()
{
	return unit_helper.attr("UnitAirMoveCommand")();
}
#endif // ENABLE_PYTHON

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void UnitBaseAirLocomotion::Move( float interval, UnitBaseMoveCommand &move_command )
{
	VPROF_BUDGET( "UnitBaseAirLocomotion::Move", VPROF_BUDGETGROUP_UNITS );

	mv = &move_command;

	mv->interval = interval;
	mv->outwishvel.Init();

	Friction();
	FullAirMove();
	MoveFacing();

	// Sometimes an unit spawns with an invalid velocity and can't move
	// Validate velocity after each move
	CheckVelocity();
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void UnitBaseAirLocomotion::FinishMove( UnitBaseMoveCommand &mv )
{
	// Hack: clamp z to max height (Strider)
	if( m_fMaxHeight > 0 && m_fCurrentHeight >= m_fMaxHeight )
	{
		mv.origin.z -= (m_fCurrentHeight - m_fMaxHeight);
	}

	UnitBaseLocomotion::FinishMove( mv );

	UnitAirMoveCommand *pAirMoveCommand = dynamic_cast<UnitAirMoveCommand *>( &mv );
	if( pAirMoveCommand )
	{
		pAirMoveCommand->height = m_fCurrentHeight;
		pAirMoveCommand->desiredheight = m_fDesiredHeight;
	}
	else
	{
		static bool s_bDidWarn = false;
		if( !s_bDidWarn )
		{
			Warning("UnitBaseAirLocomotion: Air Move command not used with air locomotion.\n");
			s_bDidWarn = true;
		}
	}
}

//-----------------------------------------------------------------------------
// Default to a simple single trace, but allow overriding.
// This is used for the strider, so it can determine the height based on the legs.
//-----------------------------------------------------------------------------
void UnitBaseAirLocomotion::UpdateCurrentHeight()
{
	trace_t pm;
	UTIL_TraceHull( mv->origin, mv->origin-Vector(0, 0, MAX_TRACE_LENGTH), WorldAlignMins(), WorldAlignMaxs(),
		MASK_NPCWORLDSTATIC, m_pOuter, GetOuter()->CalculateIgnoreOwnerCollisionGroup(), &pm );
	m_fCurrentHeight = fabs( pm.endpos.z-mv->origin.z );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void UnitBaseAirLocomotion::FullAirMove()
{
	CategorizePosition();
	UpdateCurrentHeight();

	if( mv->upmove == 0 )
	{
		// Get Ground distance and compare to desired height. Modify up/down velocity based on it.
		float fDiff = m_fDesiredHeight - m_fCurrentHeight;
		if( fDiff < 0.0f )
			mv->velocity.z = Max(fDiff, -mv->maxspeed); 
		else
			mv->velocity.z = Min(fDiff, mv->maxspeed);

		if( m_fFlyNoiseZ > DIST_EPSILON && mv->sidemove == 0.0f && mv->forwardmove == 0.0f  )
		{
			if( m_bFlyNoiseUp )
			{
				m_fFlyCurNoise += m_fFlyNoiseRate * mv->interval;
				if( m_fFlyCurNoise > m_fFlyNoiseZ )
					m_bFlyNoiseUp = !m_bFlyNoiseUp;
			}
			else
			{
				m_fFlyCurNoise -= m_fFlyNoiseRate * mv->interval;
				if( m_fFlyCurNoise < -m_fFlyNoiseZ )
					m_bFlyNoiseUp = !m_bFlyNoiseUp;
			}
			mv->velocity.z += m_fFlyCurNoise;
		}
	}
	else
	{
		mv->velocity.z = Min(mv->upmove, mv->maxspeed);
	}

	// Always air move
	AirMove();

	mv->stopdistance = GetStopDistance();

	// If we are on ground, no downward velocity.
	if( GetGroundEntity() != NULL)
		mv->velocity.z = 0.0f;
}

//-----------------------------------------------------------------------------
// Air acceleration is not capped for air moving units.
//-----------------------------------------------------------------------------
void UnitBaseAirLocomotion::AirAccelerate( Vector& wishdir, float wishspeed, float accel )
{
	int i;
	float addspeed, accelspeed, currentspeed;
	float wishspd;

	wishspd = wishspeed;

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
	for ( i = 0 ; i < 3 ; i++)
	{
		mv->velocity[i] += accelspeed * wishdir[i];
		mv->outwishvel[i] += accelspeed * wishdir[i];
	}
}

//-----------------------------------------------------------------------------
// If I were to stop moving, how much distance would I walk before I'm halted?
//-----------------------------------------------------------------------------
float UnitBaseAirLocomotion::GetStopDistance()
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

		// apply friction
		friction = sv_friction.GetFloat() * surfacefriction;

		// Bleed off some speed, but if we have less than the bleed
		//  threshold, bleed the threshold amount.
		control = (speed < sv_stopspeed.GetFloat()) ? sv_stopspeed.GetFloat() : speed;

		// Add the amount to the drop amount.
		drop += control * friction * mv->interval;

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
// Purpose: 
//-----------------------------------------------------------------------------
void UnitBaseAirLocomotion::Friction( void )
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

	// Air units always encounter friction
	friction = sv_friction.GetFloat() * surfacefriction;

	// Bleed off some speed, but if we have less than the bleed
	//  threshold, bleed the threshold amount.
	control = (speed < sv_stopspeed.GetFloat()) ? sv_stopspeed.GetFloat() : speed;

	// Add the amount to the drop amount.
	drop += control * friction * mv->interval;

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
}
