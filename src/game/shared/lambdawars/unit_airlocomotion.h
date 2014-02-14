//====== Copyright © Sandern Corporation, All rights reserved. ===========//
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

class UnitAirMoveCommand : public UnitBaseMoveCommand
{
public:
	float height;
	float desiredheight;
};

class UnitBaseAirLocomotion : public UnitBaseLocomotion
{
public:
#ifdef ENABLE_PYTHON
	UnitBaseAirLocomotion( boost::python::object outer );

	virtual boost::python::object	CreateMoveCommand();
#endif // ENABLE_PYTHON

	virtual void	Move( float interval, UnitBaseMoveCommand &mv );

	virtual void	FinishMove( UnitBaseMoveCommand &mv );

	virtual void	FullAirMove();
	virtual void	UpdateCurrentHeight();

	// Acceleration
	virtual void	AirAccelerate( Vector& wishdir, float wishspeed, float accel );

	// Friction
	void			Friction( void );

	// If I were to stop moving, how much distance would I walk before I'm halted?
	virtual float	GetStopDistance();

public:
	float m_fCurrentHeight;
	float m_fDesiredHeight;
	float m_fMaxHeight;
	float m_fFlyNoiseZ;
	float m_fFlyNoiseRate;

protected:
	float m_fFlyCurNoise;
	bool m_bFlyNoiseUp;
};

#endif // UNIT_AIRLOCOMOTION_H