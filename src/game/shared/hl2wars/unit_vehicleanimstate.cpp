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

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#ifdef ENABLE_PYTHON
UnitVehicleAnimState::UnitVehicleAnimState( boost::python::object outer ) : UnitBaseAnimState( outer )
{
	m_iVehicleSteer = -1;
	m_iVehicleFLSpin = -1;
	m_iVehicleFRSpin = -1;
	m_iVehicleRLSpin = -1;
	m_iVehicleRRSpin = -1;

	m_fWheelRadius = 22.0f;
}
#endif // ENABLE_PYTHON

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const QAngle& UnitVehicleAnimState::GetRenderAngles()
{
	return m_angRender;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnitVehicleAnimState::Update( float eyeYaw, float eyePitch )
{
	m_angRender[YAW] = eyeYaw - wars_vehicle_yaw_adjustment.GetFloat();
	m_angRender[PITCH] = eyePitch;
	m_angRender[ROLL] = 0.0f;

	UpdateSteering();
	UpdateWheels();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnitVehicleAnimState::UpdateSteering()
{
	if( m_iVehicleSteer == -1 )
		return;

	Vector vel, forward, right;
	GetOuter()->GetVectors( &forward, &right, NULL );
	GetOuterAbsVelocity( vel );
	vel.z = 0;

	float fDirRad = 0; // Default to forward steering
	float fSpeed = VectorNormalize( vel );
	if( fSpeed > 10.0f )
	{
		// Calculate steering degrees
		float fDot = DotProduct( forward, vel );
		fDirRad = acos( fDot );
		
		// Calculate direction of steer
		fDot = DotProduct( vel, right );
		if( fDot > 0 )
			fDirRad *= -1;
	}

	// Converge to the new steering target
	float targetSteer = fDirRad * 2.0f;
	float prevSteer = GetOuter()->GetPoseParameter( m_iVehicleSteer );
	float newSteer = prevSteer;
	if( prevSteer < targetSteer )
		newSteer = Min( prevSteer + 5.0f * GetAnimTimeInterval(), targetSteer );
	else
		newSteer = Max( prevSteer - 5.0f * GetAnimTimeInterval(), targetSteer );

	SetOuterPoseParameter( m_iVehicleSteer, newSteer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnitVehicleAnimState::UpdateWheels()
{
	// Determine ideal playback rate
	Vector vel, forward;
	GetOuter()->GetVectors( &forward, NULL, NULL );
	GetOuterAbsVelocity( vel );
	vel.z = 0;

	float fSpeed = VectorNormalize( vel );

	float wheelAdvancement = fSpeed * (360.0f / (2 * m_fWheelRadius * M_PI)) * GetAnimTimeInterval();

	// Inverse wheel advancement direction when moving backwards
	float fDot = DotProduct( forward, vel );
	if( fDot < 0 )
		wheelAdvancement *= -1;

	UpdateWheel( m_iVehicleFLSpin, wheelAdvancement );
	UpdateWheel( m_iVehicleFRSpin, wheelAdvancement );
	UpdateWheel( m_iVehicleRLSpin, wheelAdvancement );
	UpdateWheel( m_iVehicleRRSpin, wheelAdvancement );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnitVehicleAnimState::UpdateWheel( int iParam, float wheelAdvancement )
{
	if( iParam == -1 )
		return;

	float curWheelState = GetOuter()->GetPoseParameter( iParam );

	curWheelState += wheelAdvancement;

	SetOuterPoseParameter( iParam, curWheelState );
}