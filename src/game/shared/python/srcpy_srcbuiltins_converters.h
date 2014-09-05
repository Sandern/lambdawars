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

namespace bp = boost::python;

#if 0 // TODO
// ---------------------------------------------------------------------------------------------------------
// -- KeyValues converter
struct ptr_keyvalues_to_py_keyvalues : boost::python::to_python_converter<KeyValues *, ptr_keyvalues_to_py_keyvalues>
{
	static PyObject* convert(KeyValues *s)
	{
		if( s ) {
			return boost::python::incref(boost::python::object(PyKeyValues(s)).ptr());
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

#endif // 0

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
#if PY_VERSION_HEX < 0x03000000
		if( !PyString_Check( obj_ptr ) )
			return 0;
#else
		if( !PyUnicode_Check( obj_ptr ) )
			return 0;
#endif
		return obj_ptr;
	}

	static void construct(
		PyObject* obj_ptr,
		boost::python::converter::rvalue_from_python_stage1_data* data)
	{
#if PY_VERSION_HEX < 0x03000000
		char* value = PyString_AsString( obj_ptr );
#else
		PyObject *pDecoded = PyUnicode_AsUTF8String( obj_ptr );
		char* value = PyBytes_AsString( pDecoded );
		Py_DECREF( pDecoded );
#endif

		if (value == 0) 
		{ 
			boost::python::throw_error_already_set();
		}
		void* storage = ((boost::python::converter::rvalue_from_python_storage<string_t>*)data)->storage.bytes;
#ifndef CLIENT_DLL
		string_t s = AllocPooledString(value);
		memcpy(storage, &s, sizeof(string_t));
#else
		boost::python::throw_error_already_set();
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
#if PY_VERSION_HEX < 0x03000000
		if( !PyString_Check( obj_ptr ) )
			return 0;
#else
		if( !PyUnicode_Check( obj_ptr ) ) 
			return 0;
#endif
		return obj_ptr;
	}

	static void construct(
		PyObject* obj_ptr,
		boost::python::converter::rvalue_from_python_stage1_data* data)
	{
#if PY_VERSION_HEX < 0x03000000
		char* strvalue = PyString_AsString( obj_ptr );
#else
		PyObject *pDecoded = PyUnicode_AsUTF8String( obj_ptr );
		char* strvalue = PyBytes_AsString( pDecoded );
		Py_DECREF( pDecoded );
#endif

		if (strvalue == 0) 
		{ 
			boost::python::throw_error_already_set();
			return;
		}
		void* storage = ((boost::python::converter::rvalue_from_python_storage<wchar_t>*)data)->storage.bytes;
		new (storage) wchar_t(strvalue[0]);
		data->convertible = storage;
	}
};

#if PY_VERSION_HEX < 0x03000000
struct python_unicode_to_ptr_const_wchar_t
{
	python_unicode_to_ptr_const_wchar_t()
	{
		boost::python::converter::registry::insert(
			&convert_to_wcstring, 
			boost::python::type_id<wchar_t>(),
			&boost::python::converter::wrap_pytype<&PyString_Type>::get_pytype
		);
	}

	static void *convert_to_wcstring(PyObject* obj)
	{
		if( PyUnicode_Check( obj ) )
		{
			return PyUnicode_AsUnicode(obj);
		}
		return 0;
	}
};
#endif

#endif // SRCPYTHON_CONVERTERS_H