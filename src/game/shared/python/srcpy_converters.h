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

#include <boost/python.hpp>

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

// ---------------------------------------------------------------------------------------------------------
// -- string_t converter
struct string_t_to_python_str
{
	static PyObject* convert(string_t const& s)
	{
#ifdef NO_STRING_T
		return boost::python::incref(boost::python::object((const char *)s).ptr());
#else
		return boost::python::incref(boost::python::object(s.ToCStr()).ptr());
#endif
	}
};

struct python_str_to_string_t
{
	python_str_to_string_t()
	{
		boost::python::converter::registry::push_back(
			&convertible,
			&construct,
			boost::python::type_id<string_t>());
	}

	static void* convertible(PyObject* obj_ptr)
	{
		if (!PyString_Check(obj_ptr)) return 0;
		return obj_ptr;
	}

	static void construct(
		PyObject* obj_ptr,
		boost::python::converter::rvalue_from_python_stage1_data* data)
	{
		char* value = PyString_AsString(obj_ptr);
		if (value == 0) { 
			boost::python::throw_error_already_set();
		}
		void* storage = ((boost::python::converter::rvalue_from_python_storage<string_t>*)data)->storage.bytes;
#ifndef CLIENT_DLL
		//new (storage) castable_string_t(value);
		string_t s = AllocPooledString(value);
		memcpy(storage, &s, sizeof(string_t));
#else
		boost::python::throw_error_already_set();
		//new (storage) char[];
		//memcpy(storage, value, )
#endif // CLIENT_DLL
		data->convertible = storage;
	}
};

// ---------------------------------------------------------------------------------------------------------
// -- wchar_t converter
struct wchar_t_to_python_str
{
	static PyObject* convert(wchar_t const& s)
	{
		return boost::python::incref(PyUnicode_FromWideChar(&s, 1));
	}
};

struct ptr_wchar_t_to_python_str : boost::python::to_python_converter<wchar_t *, ptr_wchar_t_to_python_str>
{
	static PyObject* convert(wchar_t *s)
	{
		if( s ) {
			return boost::python::incref(PyUnicode_FromWideChar(s, wcslen(s)));
		}
		else {
			return boost::python::incref(Py_None);
		}
	}
};

struct python_str_to_wchar_t
{
	python_str_to_wchar_t()
	{
		boost::python::converter::registry::push_back(
			&convertible,
			&construct,
			boost::python::type_id<wchar_t>());
	}

	static void* convertible(PyObject* obj_ptr)
	{
		if (!PyString_Check(obj_ptr)) return 0;
		return obj_ptr;
	}

	static void construct(
		PyObject* obj_ptr,
		boost::python::converter::rvalue_from_python_stage1_data* data)
	{
		char* strvalue = PyString_AsString(obj_ptr);
		if (strvalue == 0) { 
			boost::python::throw_error_already_set();
			return;
		}
		void* storage = ((boost::python::converter::rvalue_from_python_storage<wchar_t>*)data)->storage.bytes;
		new (storage) wchar_t(strvalue[0]);
		//*storage = 
		data->convertible = storage;
	}
};

// ---------------------------------------------------------------------------------------------------------
// -- IPhysicsObjectHandle converter
/*
struct iphysicsobjecthandle_to_iphysicsobject
{
	iphysicsobjecthandle_to_iphysicsobject()
	{
		boost::python::converter::registry::insert(
			&extract_iphysicsobjecthandle, 
			boost::python::type_id<IPhysicsObjectHandle>()
		);
	}

	static void* extract_iphysicsobjecthandle(PyObject* op){
		boost::python::object handle = boost::python::object(
			boost::python::handle<>(
				boost::python::borrowed(op)
			)
		);
		IPhysicsObjectHandle pohandle =  boost::python::extract<IPhysicsObjectHandle>(handle);
		return pohandle.VPhysicsGetObject();
	}
};


// ---------------------------------------------------------------------------------------------------------
// -- IPhysicsObjectHandle converter
struct iphysicsshadowcontrollerhandle_to_iphysicsshadowcontroller
{
	iphysicsshadowcontrollerhandle_to_iphysicsshadowcontroller()
	{
		boost::python::converter::registry::insert(
			&extract_iphysicsshadowcontrollerhandle, 
			boost::python::type_id<IPhysicsShadowControllerHandle>()
			);
	}

	static void* extract_iphysicsshadowcontrollerhandle(PyObject* op){
		boost::python::object handle = boost::python::object(
			boost::python::handle<>(
				boost::python::borrowed(op)
			)
		);
		IPhysicsShadowControllerHandle pohandle =  boost::python::extract<IPhysicsShadowControllerHandle>(handle);
		return pohandle.GetShadowController();
	}
};
*/

// ---------------------------------------------------------------------------------------------------------
// -- ray_t converter
// FIXME: Can't do this due vector align in the Ray_t struct...
/*
struct ray_t_to_python_ray
{
	static PyObject* convert(Ray_t const& r)
	{
		return boost::python::incref(boost::python::object(PyRay_t(r)).ptr());
	}
};
*/

#endif // SRCPYTHON_CONVERTERS_H