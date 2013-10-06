//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SRCPYTHON_CONVERTERS_ENTS_H
#define SRCPYTHON_CONVERTERS_ENTS_H

#ifdef _WIN32
#pragma once
#endif

#include "srcpy_boostpython.h"

#include "imouse.h"

namespace bp = boost::python;

// ---------------------------------------------------------------------------------------------------------
// -- IMouse converter
struct ptr_imouse_to_py_imouse : boost::python::to_python_converter<IMouse *, ptr_imouse_to_py_imouse>
{
	static PyObject* convert(IMouse *s)
	{
		if( s ) {
			CBaseEntity *pEnt = dynamic_cast<CBaseEntity *>(s);		// Might want to use a less expensive check
			if( pEnt == NULL )
				return Py_None;
			if( pEnt->GetPyInstance().ptr() != Py_None )
				return boost::python::incref(pEnt->GetPyInstance().ptr());
			return boost::python::incref(pEnt->CreatePyHandle().ptr());
		}
		else {
			return boost::python::incref(Py_None);
		}
	}
};


#endif // SRCPYTHON_CONVERTERS_ENTS_H