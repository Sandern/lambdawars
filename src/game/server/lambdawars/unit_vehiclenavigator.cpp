//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "unit_vehiclenavigator.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar unit_vehicle_angle_tolerance("unit_vehicle_angle_tolerance", "15.0");
ConVar unit_vehicle_waypoint_tolerance("unit_vehicle_waypoint_tolerance", "16.0");
ConVar unit_vehicle_min_dist_turnmod("unit_vehicle_min_dist_turnmod", "4.0");

#ifdef ENABLE_PYTHON
UnitVehicleNavigator::UnitVehicleNavigator( boost::python::object outer ) : UnitBaseNavigator( outer )
{

}
#endif // ENABLE_PYTHON

//-----------------------------------------------------------------------------
// Purpose: Updates our preferred facing direction.
//			Defaults to the path direction.
//-----------------------------------------------------------------------------
void UnitVehicleNavigator::UpdateIdealAngles( UnitBaseMoveCommand &MoveCommand, Vector *pPathDir )
{
	float fSpeed = GetWishVelocity().Length2D();
	if( fSpeed > 10 )
	{
		if( pPathDir ) 
		{
			VectorAngles(*pPathDir, MoveCommand.idealviewangles);
		}
	}
	else
	{
		MoveCommand.idealviewangles = GetAbsAngles();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Calculates the allowed move directions and densities in those directions
//			In case of a vehicle, the only possible directions are forward and backward
//			TODO: Allow some turning directions when moving?
//-----------------------------------------------------------------------------
void UnitVehicleNavigator::ComputeConsiderDensAndDirs( UnitBaseMoveCommand &MoveCommand, Vector &vPathDir, CheckGoalStatus_t GoalStatus )
{
	//float fSpeed = GetWishVelocity().Length2D();

	// Reset list information
	m_iUsedTestDirections = 0;

	const Vector &origin = GetAbsOrigin();
	float fRadius = GetEntityBoundingRadius(m_pOuter);

	// Try testing a bit further in case we are stuck
	if( GetBlockedStatus() >= BS_MUCH )
		fRadius *= 3.0f;
	else if( GetBlockedStatus() >= BS_LITTLE )
		fRadius *= 2.0f;

	// Calculate forward and backward direction
	m_pOuter->GetVectors(&m_vTestDirections[m_iUsedTestDirections], NULL, NULL); // Just use forward as start dir
	m_vTestPositions[m_iUsedTestDirections] = origin + (m_vTestDirections[m_iUsedTestDirections] * fRadius);
	ComputeDensityAndAvgVelocity( m_iUsedTestDirections, MoveCommand );
	m_iUsedTestDirections += 1;

	m_vTestDirections[m_iUsedTestDirections] = -m_vTestDirections[m_iUsedTestDirections-1];
	m_vTestPositions[m_iUsedTestDirections] = origin + (m_vTestDirections[m_iUsedTestDirections] * fRadius);
	ComputeDensityAndAvgVelocity( m_iUsedTestDirections, MoveCommand );
	m_iUsedTestDirections += 1;

	// Calculate turning direction in case we are moving
	/*if( fSpeed > 10.0f )
	{

	}*/
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnitVehicleNavigator::CalcMove( UnitBaseMoveCommand &MoveCommand, QAngle angles, float speed )
{
	if( GetPath()->m_iGoalType != GOALTYPE_NONE )
	{
		// Check if we need to back the vehicle to make a turn (estimated)
		UnitBaseWaypoint *pCurWaypoint = GetPath()->m_pWaypointHead;
		if( pCurWaypoint )
		{
			Vector vDir;
			float waypointDist = UnitComputePathDirection( GetAbsOrigin(), pCurWaypoint->GetPos(), vDir );
			QAngle turnAngles;
			VectorAngles( vDir, turnAngles );
			float angle = anglemod( MoveCommand.viewangles[YAW] - turnAngles[YAW] );
			if( angle > 180.0f )
				angle = fabs( angle - 360 );

			float distNeeded = CalcNeededDistanceForTurn( MoveCommand, angle );
			if( angle > unit_vehicle_angle_tolerance.GetFloat() &&
				waypointDist > unit_vehicle_waypoint_tolerance.GetFloat() && 
				distNeeded * unit_vehicle_min_dist_turnmod.GetFloat() > waypointDist )
			{
				MoveCommand.forwardmove = -Min( distNeeded, MoveCommand.maxspeed );
				MoveCommand.sidemove = 0;
				return;
			}
		}
	}

	BaseClass::CalcMove( MoveCommand, angles, speed );
}

//-----------------------------------------------------------------------------
// Purpose: Returns a very rough estimate for distance needed to turn
//-----------------------------------------------------------------------------
float UnitVehicleNavigator::CalcNeededDistanceForTurn( UnitBaseMoveCommand &MoveCommand, float turn )
{
	float turnPerInterval = MoveCommand.yawspeed * 10.0f * MoveCommand.interval;
	float turnStep, speedWeight;

	Vector dir( 1, 0, 0 );
	Vector end(0, 0, 0);
	int i = 0;
	for( i = 0; turn > 0 && i < 1000; i++ )
	{
		turnStep = Min( turnPerInterval, turn - turnPerInterval );
		speedWeight = turnStep / turnPerInterval;
		turn -= turnPerInterval;
		VectorYawRotate( dir, turnStep, dir );
		end += dir * (MoveCommand.maxspeed * speedWeight * MoveCommand.interval);
	}
	return end.Length2D();
}