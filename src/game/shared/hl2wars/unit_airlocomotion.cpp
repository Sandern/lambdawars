//====== Copyright © 2013 Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "unit_airlocomotion.h"
#include "movevars_shared.h"
#include "coordsize.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
#ifdef ENABLE_PYTHON
UnitBaseAirLocomotion::UnitBaseAirLocomotion( boost::python::object outer )
	:UnitBaseLocomotion(outer)
{
	m_fDesiredHeight = 490.0f;
	m_fFlyNoiseZ = 0.0f;
	m_fFlyNoiseRate = 0.0f;
	m_fFlyCurNoise = 0.0f;
	m_bFlyNoiseUp = true;
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
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void UnitBaseAirLocomotion::FullAirMove()
{
	CategorizePosition();

	if( mv->upmove == 0.0f )
	{
		// Get Ground distance and compare to desired height. Modify up/down velocity based on it.
		trace_t pm;
		UTIL_TraceHull( mv->origin, mv->origin-Vector(0, 0, MAX_TRACE_LENGTH), WorldAlignMins(), WorldAlignMaxs(),
			MASK_NPCWORLDSTATIC, m_pOuter, GetOuter()->CalculateIgnoreOwnerCollisionGroup(), &pm );
		float fGroundDist = fabs(pm.endpos.z-mv->origin.z);
		float fDiff = m_fDesiredHeight - fGroundDist;
		if( fDiff < 0.0f )
			mv->velocity.z = MAX(fDiff, -mv->maxspeed); 
		else
			mv->velocity.z = MIN(fDiff, mv->maxspeed);

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
		mv->velocity.z = MIN(mv->upmove, mv->maxspeed);
	}

	// Always air move
	AirMove();

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
	for (i=0 ; i<3 ; i++)
	{
		mv->velocity[i] += accelspeed * wishdir[i];
		mv->outwishvel[i] += accelspeed * wishdir[i];
	}
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
	//if (m_pOuter->GetGroundEntity() != NULL)  // On an entity that is the ground
	{
		friction = sv_friction.GetFloat() * surfacefriction;

		// Bleed off some speed, but if we have less than the bleed
		//  threshold, bleed the threshold amount.
		control = (speed < sv_stopspeed.GetFloat()) ? sv_stopspeed.GetFloat() : speed;

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
}
