//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
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
#ifndef DISABLE_PYTHON
UnitBaseAnimState::UnitBaseAnimState(boost::python::object outer) : UnitComponent(outer)
{
#ifdef CLIENT_DLL
	if( m_pOuter->m_pAnimState )
		Warning("#%d: Clearing duplicate animstate\n", m_pOuter->entindex());
	m_pOuter->m_pAnimState = this;
#endif // CLIENT_DLL
}
#endif // DISABLE_PYTHON

UnitBaseAnimState::~UnitBaseAnimState()
{
#ifdef CLIENT_DLL
	m_pOuter->m_pAnimState = NULL;
#endif // CLIENT_DLL
}

float UnitBaseAnimState::GetAnimTimeInterval( void ) const
{
#ifdef CLIENT_DLL
	return gpGlobals->frametime;
#else
	return GetOuter()->GetAnimTimeInterval();
#endif // CLIENT_DLL
}