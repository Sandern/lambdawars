//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
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
// Purpose: Python implementation of the temp entity interface
//-----------------------------------------------------------------------------
class PyTempEnts : public CTempEnts
{
public:
	virtual void				Clear( void );
	virtual void				Update( void );

	virtual boost::python::object PySpawnTempModel( model_t *pModel, const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecVelocity, float flLifeTime, int iFlags );
	virtual boost::python::object PySpawnTempModel( const char *model_name, const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecVelocity, float flLifeTime, int iFlags );

	// Internal methods also available to children
protected:
	C_LocalTempEntity		*PyTempEntAlloc( const Vector& org, model_t *model );

private:
	CUtlVector<boost::python::object> m_pyTempEnts;
};

extern PyTempEnts *pytempents;

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

#endif // CLIENT_DLL

#endif // SRC_PYTHON_TE_H