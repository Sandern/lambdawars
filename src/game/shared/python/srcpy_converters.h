//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SRCPYTHON_CONVERTERS_H
#define SRCPYTHON_CONVERTERS_H

#ifdef _WIN32
#pragma once
#endif

#include "srcpy_boostpython.h"

#include "srcpy_base.h"

namespace bp = boost::python;

// ---------------------------------------------------------------------------------------------------------
// -- KeyValues converter
struct ptr_keyvalues_to_py_keyvalues : boost::python::to_python_converter<KeyValues *, ptr_keyvalues_to_py_keyvalues>
{
	static PyObject* convert(KeyValues *s)
	{
		if( s ) {
			return boost::python::incref(boost::python::object(PyKeyValues(s)).ptr());
			//return boost::python::incref(PyKeyValuesToDict(s).ptr());
		}
		else {
			return boost::python::incref(Py_None);
		}
	}
};

struct keyvalues_to_py_keyvalues : boost::python::to_python_converter<KeyValues, keyvalues_to_py_keyvalues>
{
	static PyObject* convert(const KeyValues &s)
	{
		return boost::python::incref(boost::python::object(PyKeyValues(&s)).ptr());
		//return boost::python::incref(PyKeyValuesToDict(&s).ptr()); //
	}
};

extern PyTypeObject *g_PyKeyValuesType;
struct py_keyvalues_to_keyvalues
{
	py_keyvalues_to_keyvalues()
	{
		boost::python::converter::registry::insert(
			&extract_keyvalues, 
			boost::python::type_id<KeyValues>()
			);
	}

	static void* extract_keyvalues(PyObject* op){
		//if ( Q_strncmp( Py_TYPE(op)->tp_name, "KeyValues", 9 ) )
#if 1
		if( Py_TYPE(op) != g_PyKeyValuesType )
			return (void *)0;
		boost::python::object handle = boost::python::object(
			boost::python::handle<>(
				boost::python::borrowed(op)
			)
		);
		return boost::python::extract<KeyValues *>(handle.attr("__GetRawKeyValues")());
		
#else
		boost::python::object handle = boost::python::object(
			boost::python::handle<>(
				boost::python::borrowed(op)
			)
		);
		return PyDictToKeyValues( bp::dict( handle ) );
#endif // 0
	}
};

#endif // SRCPYTHON_CONVERTERS_H