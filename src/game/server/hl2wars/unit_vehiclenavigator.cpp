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
