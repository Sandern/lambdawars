//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================//

#ifndef UNIT_VEHICLEANIMSTATE_H
#define UNIT_VEHICLEANIMSTATE_H
#ifdef _WIN32
#pragma once
#endif

#include "unit_baseanimstate.h"

class UnitVehicleAnimState : public UnitBaseAnimState
{
public:
	DECLARE_CLASS( UnitVehicleAnimState, UnitBaseAnimState );

#ifdef ENABLE_PYTHON
	UnitVehicleAnimState( boost::python::object outer );
#endif // ENABLE_PYTHON

	virtual const QAngle& GetRenderAngles();

	virtual void Update( float eyeYaw, float eyePitch );

	virtual void UpdateSteering();
	virtual void UpdateWheels();

	void UpdateWheel( int iParam, float wheelAdvancement );

public:
	int m_iVehicleSteer;

	int m_iVehicleFLSpin;
	int m_iVehicleFRSpin;
	int m_iVehicleRLSpin;
	int m_iVehicleRRSpin;

	float m_fWheelRadius;

private:
	QAngle m_angRender;
};

#endif // UNIT_VEHICLEANIMSTATE_H