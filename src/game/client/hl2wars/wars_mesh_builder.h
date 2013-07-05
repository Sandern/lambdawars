//====== Copyright © Sandern Corporation, All rights reserved. ===========//
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

#ifdef ENABLE_PYTHON

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

#endif // ENABLE_PYTHON

#endif // WARS_MESH_BUILDER_H