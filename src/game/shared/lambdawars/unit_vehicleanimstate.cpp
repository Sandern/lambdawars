//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "unit_vehicleanimstate.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

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

	m_fFrontWheelRadius = 18.0f;
	m_fRearWheelRadius = 22.0f;
}
#endif // ENABLE_PYTHON

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnitVehicleAnimState::CalcWheelData()
{
#ifdef CLIENT_DLL
	C_BaseAnimating::PushAllowBoneAccess( true, false, "UnitVehicleAnimState::CalcWheelData" );
#endif // CLIENT_DLL

	const char *pWheelAttachments[4] = { "wheel_fl", "wheel_fr", "wheel_rl", "wheel_rr" };
	Vector left, right;
	QAngle dummy;
	SetOuterPoseParameter( m_iVehicleFLHeight, 0 );
	SetOuterPoseParameter( m_iVehicleFRHeight, 0 );
	SetOuterPoseParameter( m_iVehicleRLHeight, 0 );
	SetOuterPoseParameter( m_iVehicleRRHeight, 0 );
	m_pOuter->InvalidateBoneCache();

	if ( m_pOuter->GetAttachment( "wheel_fl", left, dummy ) && m_pOuter->GetAttachment( "wheel_fr", right, dummy ) )
	{
		VectorITransform( left, m_pOuter->EntityToWorldTransform(), left );
		VectorITransform( right, m_pOuter->EntityToWorldTransform(), right );
		//Vector center = (left + right) * 0.5;
		//vehicle.axles[0].offset = center;
		//vehicle.axles[0].wheelOffset = right - center;
		// Cache the base height of the wheels in body space
		m_wheelBaseHeight[0] = left.z;
		m_wheelBaseHeight[1] = right.z;
		m_vOffsetWheelFL = left;
		m_vOffsetWheelFR = right;
	}

	if ( m_pOuter->GetAttachment( "wheel_rl", left, dummy ) && m_pOuter->GetAttachment( "wheel_rr", right, dummy ) )
	{
		VectorITransform( left, m_pOuter->EntityToWorldTransform(), left );
		VectorITransform( right, m_pOuter->EntityToWorldTransform(), right );
		//Vector center = (left + right) * 0.5;
		//vehicle.axles[1].offset = center;
		//vehicle.axles[1].wheelOffset = right - center;
		// Cache the base height of the wheels in body space
		m_wheelBaseHeight[2] = left.z;
		m_wheelBaseHeight[3] = right.z;
		m_vOffsetWheelRL = left;
		m_vOffsetWheelRR = right;
	}
	SetOuterPoseParameter( m_iVehicleFLHeight, 1 );
	SetOuterPoseParameter( m_iVehicleFRHeight, 1 );
	SetOuterPoseParameter( m_iVehicleRLHeight, 1 );
	SetOuterPoseParameter( m_iVehicleRRHeight, 1 );
	m_pOuter->InvalidateBoneCache();
	if ( m_pOuter->GetAttachment( "wheel_fl", left, dummy ) && m_pOuter->GetAttachment( "wheel_fr", right, dummy ) )
	{
		VectorITransform( left, m_pOuter->EntityToWorldTransform(), left );
		VectorITransform( right, m_pOuter->EntityToWorldTransform(), right );
		// Cache the height range of the wheels in body space
		m_wheelTotalHeight[0] = m_wheelBaseHeight[0] - left.z;
		m_wheelTotalHeight[1] = m_wheelBaseHeight[1] - right.z;
		//vehicle.axles[0].wheels.springAdditionalLength = m_wheelTotalHeight[0];
	}

	if ( m_pOuter->GetAttachment( "wheel_rl", left, dummy ) && m_pOuter->GetAttachment( "wheel_rr", right, dummy ) )
	{
		VectorITransform( left, m_pOuter->EntityToWorldTransform(), left );
		VectorITransform( right, m_pOuter->EntityToWorldTransform(), right );
		// Cache the height range of the wheels in body space
		m_wheelTotalHeight[2] = m_wheelBaseHeight[0] - left.z;
		m_wheelTotalHeight[3] = m_wheelBaseHeight[1] - right.z;
		//vehicle.axles[1].wheels.springAdditionalLength = m_wheelTotalHeight[2];
	}
	for ( int i = 0; i < 4; i++ )
	{
		if ( m_wheelTotalHeight[i] == 0.0f )
		{
			DevWarning("Vehicle %s has invalid wheel attachment for %s - no movement\n", STRING(m_pOuter->GetModelName()), pWheelAttachments[i]);
			m_wheelTotalHeight[i] = 1.0f;
		}
	}

	SetOuterPoseParameter( m_iVehicleFLHeight, 0 );
	SetOuterPoseParameter( m_iVehicleFRHeight, 0 );
	SetOuterPoseParameter( m_iVehicleRLHeight, 0 );
	SetOuterPoseParameter( m_iVehicleRRHeight, 0 );
	m_pOuter->InvalidateBoneCache();

#ifdef CLIENT_DLL
	C_BaseAnimating::PopBoneAccess( "UnitVehicleAnimState::CalcWheelData" );
#endif // CLIENT_DLL
}

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
	m_angRender[YAW] = eyeYaw - m_pOuter->m_fModelYawRotation;
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
	const Vector &vOrigin = GetAbsOrigin();

	// Determine ideal playback rate
	Vector vel, forward;
	GetOuter()->GetVectors( &forward, NULL, NULL );
	GetOuterAbsVelocity( vel );
	vel.z = 0;

	float fSpeed = VectorNormalize( vel );

	float frontWheelAdvancement = fSpeed * (360.0f / (2 * m_fFrontWheelRadius * M_PI)) * GetAnimTimeInterval();
	float rearWheelAdvancement = fSpeed * (360.0f / (2 * m_fRearWheelRadius * M_PI)) * GetAnimTimeInterval();

	// Inverse wheel advancement direction when moving backwards
	float fDot = DotProduct( forward, vel );
	if( fDot < 0 )
	{
		frontWheelAdvancement *= -1;
		rearWheelAdvancement *= -1;
	}

	UpdateWheel( m_iVehicleFLSpin, frontWheelAdvancement );
	UpdateWheel( m_iVehicleFRSpin, frontWheelAdvancement );
	UpdateWheel( m_iVehicleRLSpin, rearWheelAdvancement );
	UpdateWheel( m_iVehicleRRSpin, rearWheelAdvancement );

	// Update wheel heights
	trace_t tr;
	float fDist;
	Vector vStart;

	vStart = vOrigin + m_vOffsetWheelFL;
	UTIL_TraceLine( vStart, vStart + Vector(0, 0, -(m_wheelTotalHeight[0] + 128.0f)), MASK_SOLID_BRUSHONLY, m_pOuter, COLLISION_GROUP_NONE, &tr );
	fDist = tr.endpos.DistTo(vStart) - m_fFrontWheelRadius;
	SetOuterPoseParameter( m_iVehicleFLHeight, Max( 0.0f, Min(1.0f, fDist / m_wheelTotalHeight[0]) ) );

	vStart = vOrigin + m_vOffsetWheelFR;
	UTIL_TraceLine( vStart, vStart + Vector(0, 0, -(m_wheelTotalHeight[1] + 128.0f)), MASK_SOLID_BRUSHONLY, m_pOuter, COLLISION_GROUP_NONE, &tr );
	fDist = tr.endpos.DistTo(vStart) - m_fFrontWheelRadius;
	SetOuterPoseParameter( m_iVehicleFRHeight, Max( 0.0f, Min(1.0f, fDist / m_wheelTotalHeight[1]) ) );

	vStart = vOrigin + m_vOffsetWheelRL;
	UTIL_TraceLine( vStart, vStart + Vector(0, 0, -(m_wheelTotalHeight[2] + 128.0f)), MASK_SOLID_BRUSHONLY, m_pOuter, COLLISION_GROUP_NONE, &tr );
	fDist = tr.endpos.DistTo(vStart) - m_fRearWheelRadius;
	SetOuterPoseParameter( m_iVehicleRLHeight, Max( 0.0f, Min(1.0f, fDist / m_wheelTotalHeight[2]) ) );

	vStart = vOrigin + m_vOffsetWheelRR;
	UTIL_TraceLine( vStart, vStart + Vector(0, 0, -(m_wheelTotalHeight[3] + 128.0f)), MASK_SOLID_BRUSHONLY, m_pOuter, COLLISION_GROUP_NONE, &tr );
	fDist = tr.endpos.DistTo(vStart) - m_fRearWheelRadius;
	SetOuterPoseParameter( m_iVehicleRRHeight, Max( 0.0f, Min(1.0f, fDist / m_wheelTotalHeight[3]) ) );
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