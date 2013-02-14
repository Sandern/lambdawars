//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "unit_component.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef ENABLE_PYTHON
UnitComponent::UnitComponent( boost::python::object outer )
{
	m_pOuter = boost::python::extract<CUnitBase *>(outer);
	if( !m_pOuter )
	{
		PyErr_SetString(PyExc_Exception, "UnitComponent: Invalid outer. Must be of type CUnitBase!" );
		throw boost::python::error_already_set(); 
		return;
	}
	m_pyOuter = outer;
}
#endif // ENABLE_PYTHON
