//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SRC_PYTHON_PARTICLES_CONVERTERS_H
#define SRC_PYTHON_PARTICLES_CONVERTERS_H
#ifdef _WIN32
#pragma once
#endif

#include "src_python_particles.h"

struct ptr_newparticleeffect_to_handle : boost::python::to_python_converter<CNewParticleEffect *, ptr_newparticleeffect_to_handle>
{
	static PyObject* convert(CNewParticleEffect *s)
	{
		if( s ) {
			return boost::python::incref(boost::python::object(CNewParticleEffectHandle(s)).ptr());
		}
		else {
			return boost::python::incref(Py_None);
		}
	}
};

struct handle_to_newparticleeffect
{
	handle_to_newparticleeffect()
	{
		boost::python::converter::registry::insert(
			&extract_newparticleeffect, 
			boost::python::type_id<CNewParticleEffect>()
			);
	}

	static void* extract_newparticleeffect(PyObject* op) {
		boost::python::object handle = boost::python::object(
			boost::python::handle<>(
			boost::python::borrowed(op)
			)
		);
		CNewParticleEffectHandle w = boost::python::extract<CNewParticleEffectHandle>(handle);
		return w.GetParticleEffect();
	}
};

#endif // SRC_PYTHON_PARTICLES_CONVERTERS_H