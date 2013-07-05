//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "unit_expresser.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef ENABLE_PYTHON
UnitExpresser::UnitExpresser( boost::python::object outer ) : UnitComponent(outer)
{
	SetOuter( UnitComponent::GetOuter() );
	Connect( UnitComponent::GetOuter() );
}
#endif // ENABLE_PYTHON
