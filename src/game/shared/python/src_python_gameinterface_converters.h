//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef SRC_PYTHON_TEMPENTS_CONVERTERS_H
#define SRC_PYTHON_TEMPENTS_CONVERTERS_H
#ifdef _WIN32
#pragma once
#endif

class CTempEnts;
struct model_t;

#include "src_python_gameinterface.h"

// Converters
struct ptr_model_t_to_wrap_model_t : boost::python::to_python_converter<model_t *, ptr_model_t_to_wrap_model_t>
{
	static PyObject* convert(model_t *s)
	{
		if( s ) {
			return boost::python::incref(boost::python::object(wrap_model_t(s)).ptr());
		}
		else {
			return boost::python::incref(Py_None);
		}
	}
};

struct const_ptr_model_t_to_wrap_model_t : boost::python::to_python_converter<const model_t *, const_ptr_model_t_to_wrap_model_t>
{
	static PyObject* convert(const model_t *s)
	{
		if( s ) {
			return boost::python::incref(boost::python::object(wrap_model_t((model_t *)s)).ptr());
		}
		else {
			return boost::python::incref(Py_None);
		}
	}
};

struct wrap_model_t_to_model_t
{
	wrap_model_t_to_model_t()
	{
		boost::python::converter::registry::insert(
			&extract_model_t, 
			boost::python::type_id<model_t>()
			);
	}

	static void* extract_model_t(PyObject* op) {
		//if ( Q_strncmp( Py_TYPE(op)->tp_name, "model_t", 9 ) )
		//	return (void *)0;
		boost::python::object handle = boost::python::object(
			boost::python::handle<>(
			boost::python::borrowed(op)
			)
		);
		wrap_model_t w = boost::python::extract<wrap_model_t>(handle);
		return w.pModel;
	}
};

#endif // SRC_PYTHON_TEMPENTS_CONVERTERS_H