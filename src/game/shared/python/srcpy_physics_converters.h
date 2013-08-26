//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SRCPY_PHYSICS_CONVERTERS_H
#define SRCPY_PHYSICS_CONVERTERS_H

#ifdef _WIN32
#pragma once
#endif

#include "srcpy_physics.h"

// Converters
struct ptr_IPhysicsObject_to_PyPhysicsObject : boost::python::to_python_converter<IPhysicsObject *, ptr_IPhysicsObject_to_PyPhysicsObject>
{
	static PyObject* convert(IPhysicsObject *s)
	{
		if( s ) {
			return boost::python::incref(boost::python::object(PyPhysicsObject(s)).ptr());
		}
		else {
			return boost::python::incref(Py_None);
		}
	}
};

struct const_ptr_IPhysicsObject_to_PyPhysicsObject : boost::python::to_python_converter<const IPhysicsObject *, const_ptr_IPhysicsObject_to_PyPhysicsObject>
{
	static PyObject* convert(const IPhysicsObject *s)
	{
		if( s ) {
			return boost::python::incref(boost::python::object(PyPhysicsObject((IPhysicsObject *)s)).ptr());
		}
		else {
			return boost::python::incref(Py_None);
		}
	}
};

struct PyPhysicsObject_to_IPhysicsObject
{
	PyPhysicsObject_to_IPhysicsObject()
	{
		boost::python::converter::registry::insert(
			&extract_IPhysicsObject, 
			boost::python::type_id<IPhysicsObject>()
			);
	}

	static void* extract_IPhysicsObject(PyObject* op) {
		boost::python::object handle = boost::python::object(
			boost::python::handle<>(
			boost::python::borrowed(op)
			)
		);
		PyPhysicsObject *w = boost::python::extract<PyPhysicsObject *>(handle);
		if( w )
			return w->GetVPhysicsObject();
		return NULL;
	}
};

#endif // SRCPY_PHYSICS_CONVERTERS_H