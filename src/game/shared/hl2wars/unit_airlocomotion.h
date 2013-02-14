//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose: Defines the locomotion of an air moving unit.
//
//=============================================================================//

#ifndef UNIT_AIRLOCOMOTION_H
#define UNIT_AIRLOCOMOTION_H

#ifdef _WIN32
#pragma once
#endif

#include "unit_locomotion.h"


class UnitBaseAirLocomotion : public UnitBaseLocomotion
{
public:
#ifdef ENABLE_PYTHON
	UnitBaseAirLocomotion( boost::python::object outer );
#endif // ENABLE_PYTHON

	virtual void	Move( float interval, UnitBaseMoveCommand &mv );

	virtual void	FullAirMove();

	// Acceleration
	virtual void	AirAccelerate( Vector& wishdir, float wishspeed, float accel );

	// Friction
	void	Friction( void );

public:
	float m_fDesiredHeight;
	float m_fFlyNoiseZ;
	float m_fFlyNoiseRate;

protected:
	float m_fFlyCurNoise;
	bool m_bFlyNoiseUp;
};

#endif // UNIT_AIRLOCOMOTION_H