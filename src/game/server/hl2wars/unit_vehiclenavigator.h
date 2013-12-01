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
	DECLARE_CLASS(UnitVehicleNavigator, UnitBaseNavigator);
public:
	friend class CUnitBase;

#ifdef ENABLE_PYTHON
	UnitVehicleNavigator( boost::python::object outer );
#endif // ENABLE_PYTHON

};

#endif // UNIT_VEHICLENAVIGATOR_H