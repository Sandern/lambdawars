//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:	
//
//=============================================================================//

#ifndef UNIT_VEHICLENAVIGATOR_H
#define UNIT_VEHICLENAVIGATOR_H

#ifdef _WIN32
#pragma once
#endif

#include "unit_navigator.h"

class UnitVehicleNavigator : public UnitBaseNavigator
{
	DECLARE_CLASS( UnitVehicleNavigator, UnitBaseNavigator );

public:
#ifdef ENABLE_PYTHON
	UnitVehicleNavigator( boost::python::object outer );
#endif // ENABLE_PYTHON

	virtual void		UpdateIdealAngles( UnitBaseMoveCommand &MoveCommand, Vector *pathdir = NULL );
	virtual void		CalcMove( UnitBaseMoveCommand &MoveCommand, QAngle angles, float speed );

	virtual void		ComputeConsiderDensAndDirs( UnitBaseMoveCommand &MoveCommand, Vector &vPathDir, CheckGoalStatus_t GoalStatus );

	virtual float		CalcNeededDistanceForTurn( UnitBaseMoveCommand &MoveCommand, float turn );
};

#endif // UNIT_VEHICLENAVIGATOR_H