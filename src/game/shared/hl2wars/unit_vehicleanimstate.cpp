//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "unit_vehicleanimstate.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar wars_vehicle_yaw_adjustment("wars_vehicle_yaw_adjustment", "90");

#ifdef ENABLE_PYTHON
UnitVehicleAnimState::UnitVehicleAnimState( boost::python::object outer ) : UnitBaseAnimState( outer )
{
	m_iVehicleSteer = -1;
	m_iVehicleFLSpin = -1;
	m_iVehicleFRSpin = -1;
	m_iVehicleRLSpin = -1;
	m_iVehicleRRSpin = -1;
}
#endif // ENABLE_PYTHON

const QAngle& UnitVehicleAnimState::GetRenderAngles()
{
	return m_angRender;
}

void UnitVehicleAnimState::Update( float eyeYaw, float eyePitch )
{
	m_angRender[YAW] = eyeYaw - wars_vehicle_yaw_adjustment.GetFloat();
	m_angRender[PITCH] = eyePitch;
	m_angRender[ROLL] = 0.0f;

	UpdateSteering();
	UpdateWheels();
}

void UnitVehicleAnimState::UpdateSteering()
{
	if( m_iVehicleSteer == -1 )
		return;

	// TODO: Calculate direction we are steering
	SetOuterPoseParameter( m_iVehicleSteer, 0 );
}

void UnitVehicleAnimState::UpdateWheels()
{
	// Determine ideal playback rate
	Vector vel;
	GetOuterAbsVelocity( vel );

	float wheelAdvancement = vel.Length2D() * GetAnimTimeInterval();

	UpdateWheel( m_iVehicleFLSpin, wheelAdvancement );
	UpdateWheel( m_iVehicleFRSpin, wheelAdvancement );
	UpdateWheel( m_iVehicleRLSpin, wheelAdvancement );
	UpdateWheel( m_iVehicleRRSpin, wheelAdvancement );
}

void UnitVehicleAnimState::UpdateWheel( int iParam, float wheelAdvancement )
{
	if( iParam == -1 )
		return;

	float curWheelState = GetOuter()->GetPoseParameter( iParam );

	curWheelState += wheelAdvancement;

	SetOuterPoseParameter( iParam, curWheelState );
}