//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "unit_vehiclenavigator.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

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
void UnitVehicleNavigator::ComputeConsiderDensAndDirs( Vector &vPathDir, CheckGoalStatus_t GoalStatus )
{
	int i;
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
	for( i = 0; i < m_iConsiderSize; i++ )
	{
		if( !m_ConsiderList[i].m_pEnt ) 
			continue;
		m_ConsiderList[i].positions[m_iUsedTestDirections].m_fDensity = ComputeEntityDensity(m_vTestPositions[m_iUsedTestDirections], m_ConsiderList[i].m_pEnt);
	}	
	m_iUsedTestDirections += 1;

	m_vTestDirections[m_iUsedTestDirections] = -m_vTestDirections[m_iUsedTestDirections-1];
	m_vTestPositions[m_iUsedTestDirections] = origin + (m_vTestDirections[m_iUsedTestDirections] * fRadius);
	for( i = 0; i < m_iConsiderSize; i++ )
	{
		if( !m_ConsiderList[i].m_pEnt ) 
			continue;
		m_ConsiderList[i].positions[m_iUsedTestDirections].m_fDensity = ComputeEntityDensity(m_vTestPositions[m_iUsedTestDirections], m_ConsiderList[i].m_pEnt);
	}	
	m_iUsedTestDirections += 1;

	// Calculate turning direction in case we are moving
	/*if( fSpeed > 10.0f )
	{

	}*/
}