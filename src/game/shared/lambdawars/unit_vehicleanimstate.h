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

	virtual void CalcWheelData();

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

	int m_iVehicleFLHeight;
	int m_iVehicleFRHeight;
	int m_iVehicleRLHeight;
	int m_iVehicleRRHeight;

	float m_fFrontWheelRadius;
	float m_fRearWheelRadius;

private:
	QAngle m_angRender;
	float m_wheelBaseHeight[4];
	float m_wheelTotalHeight[4];
	Vector m_vOffsetWheelFL, m_vOffsetWheelFR, m_vOffsetWheelRL, m_vOffsetWheelRR;

};

#endif // UNIT_VEHICLEANIMSTATE_H