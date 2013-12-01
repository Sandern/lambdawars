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