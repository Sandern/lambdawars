//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "unit_baseanimstate.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// ------------------------------------------------------------------------------------------------ //
// UnitBaseAnimState implementation.
// ------------------------------------------------------------------------------------------------ //
#ifdef ENABLE_PYTHON
UnitBaseAnimState::UnitBaseAnimState(boost::python::object outer) : UnitComponent(outer)
{
}
#endif // ENABLE_PYTHON

UnitBaseAnimState::~UnitBaseAnimState()
{
}

float UnitBaseAnimState::GetAnimTimeInterval( void ) const
{
#ifdef CLIENT_DLL
	return gpGlobals->frametime;
#else
	return GetOuter()->GetAnimTimeInterval();
#endif // CLIENT_DLL
}