//========= Copyright © 1996-2005, Sandern Corporation, All rights reserved. ============//
//
// Purpose: 
// TODO: Move to python folder
// $NoKeywords: $
//
//=============================================================================//

#ifndef WARS_MESH_BUILDER_H
#define WARS_MESH_BUILDER_H

#ifdef _WIN32
#pragma once
#endif

#include "clientsideeffects.h"

#ifndef DISABLE_PYTHON

// Vertex 
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
};


// Python mesh builder element
class PyMeshBuilder
{
public:
						PyMeshBuilder( const char *pMaterialName, MaterialPrimitiveType_t type = MATERIAL_POINTS );
						~PyMeshBuilder();

	void				Draw( double frametime );

	void				AddVertex( PyMeshVertex &vertex ); 

	void				SetMaterial( const char *pMaterialName );

private:
	int					GetNumPrimitives();

private:
	IMaterial *m_pMaterial;
	MaterialPrimitiveType_t m_nType;

	CUtlVector< PyMeshVertex > m_pyMeshVertices;
};

// Python client effect
class PyClientSideEffect : public CClientSideEffect
{
public:
						PyClientSideEffect( const char *name );
	virtual				~PyClientSideEffect( void );

	virtual void		Draw( double frametime );

	// Retuns whether the effect is still active
	virtual bool		IsActive( void );

	virtual void		Destroy( void );

	void AddMeshBuilder( bp::object meshbuilder );
	void RemoveMeshBuilder( bp::object meshbuilder );
	void ClearMeshBuilders();

	void AddToEffectList( bp::object effect );

private:
	struct PyMeshBuilderEntry 
	{
		bp::object m_pyRef;
		PyMeshBuilder *m_pMeshBuilder;
	};
	CUtlVector<PyMeshBuilderEntry> m_pyMeshBuilders;
};

// Must add to list
void AddToClientEffectList( bp::object effect );

#endif // DISABLE_PYTHON

#endif // WARS_MESH_BUILDER_H