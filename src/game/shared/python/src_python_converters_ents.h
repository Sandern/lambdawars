//====== Copyright © 2013 Sandern Corporation, All rights reserved. ===========//
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

#include <boost/python.hpp>

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

// ---------------------------------------------------------------------------------------------------------
// -- IHandleEntity converter
// ---------------------------------------------------------------------------------------------------------
// -- KeyValues converter
struct ptr_ihandleentity_to_pyhandle : boost::python::to_python_converter<IHandleEntity *, ptr_ihandleentity_to_pyhandle>
{
	static PyObject* convert(IHandleEntity *s)
	{
		if( s ) {
			return boost::python::incref((((CBaseEntity *)s)->CreatePyHandle()).ptr());
		}
		else {
			return boost::python::incref(Py_None);
		}
	}
};

struct py_ent_to_ihandleentity
{
	py_ent_to_ihandleentity()
	{
		boost::python::converter::registry::insert(
			&extract_ihandleentity, 
			boost::python::type_id<IHandleEntity>()
			);
	}

	static void* extract_ihandleentity(PyObject* op){
		//if ( Q_strncmp( Py_TYPE(op)->tp_name, "IHandleEntity", 9 ) )
		//	return (void *)0;
		boost::python::object handle = boost::python::object(
			boost::python::handle<>(
			boost::python::borrowed(op)
			)
		);
		CBaseEntity *pEnt = bp::extract<CBaseEntity *>(op);
		return (IHandleEntity *)pEnt;
	}
};

#endif // SRCPYTHON_CONVERTERS_ENTS_H