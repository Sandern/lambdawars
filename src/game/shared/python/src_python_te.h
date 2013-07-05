//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================//

#ifndef SRC_PYTHON_TE_H
#define SRC_PYTHON_TE_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL

#include "c_te_legacytempents.h"
#include "c_te_effect_dispatch.h"

struct model_t;

//-----------------------------------------------------------------------------
// Purpose: Registering new effects. Basically it just unregisters from the list.
//-----------------------------------------------------------------------------
class PyClientEffectRegistration
{
public:
	PyClientEffectRegistration(const char *pEffectName, boost::python::object method);
	~PyClientEffectRegistration();

public:
	char m_EffectName[128];
	PyClientEffectRegistration *m_pNext;
	boost::python::object m_pyMethod;

	static PyClientEffectRegistration *s_pHead;
};

#else

// Creates a concussive blast effect
void CreateConcussiveBlast( const Vector &origin, const Vector &surfaceNormal, CBaseEntity *pOwner, float magnitude );

#endif // CLIENT_DLL

#endif // SRC_PYTHON_TE_H