//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================//

#ifndef SRCPY_TE_H
#define SRCPY_TE_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL

#include "c_te_legacytempents.h"
#include "c_te_effect_dispatch.h"
#include "clientsideeffects.h"

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

//=============================================================================
// PyMeshVertex
//=============================================================================
class PyMeshVertex
{
public:
	PyMeshVertex() : normal(Vector(0, 0, 0)), stage(0), s(0.0f), t(0.0f), m_hEnt(NULL) {}

	Vector position;
	Vector normal;
	Color color;
	EHANDLE m_hEnt;

	int stage;
	float s, t;

	void SetEnt( CBaseEntity *pEnt ) { m_hEnt = pEnt; }
	CBaseEntity *GetEnt() { return m_hEnt.Get(); }

	virtual void Draw( CMeshBuilder &builder );
};

//=============================================================================
// Python mesh builder
//=============================================================================
class PyMeshBuilder
{
public:
						PyMeshBuilder( const char *pMaterialName, MaterialPrimitiveType_t type = MATERIAL_POINTS );
						~PyMeshBuilder();

	virtual void		Draw( double frametime );

	void				AddVertex( PyMeshVertex &vertex ); 

	void				SetMaterial( const char *pMaterialName );

protected:
	int					GetNumPrimitives();

protected:
	IMaterial *m_pMaterial;
	MaterialPrimitiveType_t m_nType;

	CUtlVector< PyMeshVertex > m_pyMeshVertices;
};

//=============================================================================
// Python rally line mesh builder
//=============================================================================
class PyMeshRallyLine : public PyMeshBuilder
{
public:
	PyMeshRallyLine( const char *pMaterialName );

	void				Init();

	void				Draw( double frametime );

	void SetEnt1( CBaseEntity *pEnt ) { m_hEnt1 = pEnt; }
	CBaseEntity *GetEnt1() { return m_hEnt1.Get(); }

	void SetEnt2( CBaseEntity *pEnt ) { m_hEnt2 = pEnt; }
	CBaseEntity *GetEnt2() { return m_hEnt2.Get(); }
public:
	Color color;
	Vector point1;
	Vector point2;
	float size;
	float texturex;
	float texturey;
	float texturexscale;
	float textureyscale;

protected:
	EHANDLE m_hEnt1;
	EHANDLE m_hEnt2;

private:
	float m_fLineLength;
};

//=============================================================================
// Python Client Side effect
//=============================================================================
class PyClientSideEffect : public CClientSideEffect
{
public:
						PyClientSideEffect( const char *name );
	virtual				~PyClientSideEffect( void );

	virtual void		Draw( double frametime );

	// Retuns whether the effect is still active
	virtual bool		IsActive( void );

	virtual void		Destroy( void );

	void AddMeshBuilder( boost::python::object meshbuilder );
	void RemoveMeshBuilder( boost::python::object meshbuilder );
	void ClearMeshBuilders();

	void AddToEffectList( boost::python::object effect );

private:
	struct PyMeshBuilderEntry 
	{
		boost::python::object m_pyRef;
		PyMeshBuilder *m_pMeshBuilder;
	};
	CUtlVector<PyMeshBuilderEntry> m_pyMeshBuilders;
};

// Must add to list
void AddToClientEffectList( boost::python::object effect );

#else

// Creates a concussive blast effect
void CreateConcussiveBlast( const Vector &origin, const Vector &surfaceNormal, CBaseEntity *pOwner, float magnitude );

#endif // CLIENT_DLL

#endif // SRCPY_TE_H