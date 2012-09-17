//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "wars_mesh_builder.h"
#ifndef DISABLE_PYTHON
	#include "src_python.h"
#endif // DISABLE_PYTHON

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef DISABLE_PYTHON
// Python mesh builder
PyMeshBuilder::PyMeshBuilder( const char *pMaterialName, MaterialPrimitiveType_t type )
	: m_nType(type)
{
	// Check material name
	if( !pMaterialName )
	{
		PyErr_SetString(PyExc_Exception, "No material name" );
		throw boost::python::error_already_set(); 
	}

	// Check material
	m_pMaterial = materials->FindMaterial(pMaterialName, NULL);
	if( !m_pMaterial )
	{
		PyErr_SetString(PyExc_Exception, "Material not found" );
		throw boost::python::error_already_set(); 
	}

	if(m_pMaterial)
		m_pMaterial->IncrementReferenceCount();
}


PyMeshBuilder::~PyMeshBuilder()
{
	if(m_pMaterial)
		m_pMaterial->DecrementReferenceCount();
}

void PyMeshBuilder::Draw( double frametime )
{
	CMatRenderContextPtr pRenderContext( materials );

	// Draw it.
	pRenderContext->Bind( m_pMaterial );

	IMesh *pMesh = pRenderContext->GetDynamicMesh();
	
	CMeshBuilder builder;
	builder.Begin(pMesh, m_nType, GetNumPrimitives());
	
	// Draw the vertex list
	Vector position;
	for( int i = 0; i < m_pyMeshVertices.Count(); i++ )
	{
		PyMeshVertex &vertex = m_pyMeshVertices[i];

		position = vertex.position;
		if( vertex.m_hEnt )
			position += vertex.m_hEnt->GetAbsOrigin();		

		builder.Position3fv( position.Base() );
		builder.Normal3fv( vertex.normal.Base() );
		builder.TexCoord2f( vertex.stage, vertex.s, vertex.t );
		builder.Color4ub( vertex.color[0], vertex.color[1], vertex.color[2], vertex.color[3] );
		builder.AdvanceVertex();
	}

	builder.End();
	pMesh->Draw();
}

int PyMeshBuilder::GetNumPrimitives()
{
	int iCount;

	// Calculate num primitives based on type
	switch( m_nType )
	{
	case MATERIAL_TRIANGLE_STRIP:
		iCount = MAX(0, m_pyMeshVertices.Count()-2);
		break;
	case MATERIAL_QUADS:
		iCount = MAX(0, m_pyMeshVertices.Count()-3);
		break;	
	default:
		iCount = m_pyMeshVertices.Count();
		break;
	}
	return iCount;
}

void PyMeshBuilder::AddVertex( PyMeshVertex &vertex )
{
	m_pyMeshVertices.AddToTail( vertex );
}

void PyMeshBuilder::SetMaterial( const char *pMaterialName )
{
	// Check material
	IMaterial *pMaterial = materials->FindMaterial(pMaterialName, NULL);
	if( !pMaterial )
	{
		PyErr_SetString(PyExc_Exception, "Material not found" );
		throw boost::python::error_already_set();
		return;
	}

	// Clear old
	if(m_pMaterial)
		m_pMaterial->DecrementReferenceCount();

	// Set new
	m_pMaterial = pMaterial;
	if(m_pMaterial)
	{
		m_pMaterial->IncrementReferenceCount();
	}
}

// Client effect
PyClientSideEffect::PyClientSideEffect( const char *name )
: CClientSideEffect( name )
{
}

PyClientSideEffect::~PyClientSideEffect( void )
{
	//Msg("Python effect cleaned up %s\n", GetName());
}

void PyClientSideEffect::Draw( double frametime )
{
	for( int i = 0; i < m_pyMeshBuilders.Count(); i++ )
	{
		m_pyMeshBuilders[i].m_pMeshBuilder->Draw(frametime);

	}
}

bool PyClientSideEffect::IsActive( void )
{
	return CClientSideEffect::IsActive() && Py_REFCNT(m_pyRef.ptr()) > 1;
}

void PyClientSideEffect::Destroy( void )
{
	CClientSideEffect::Destroy();
	SrcPySystem()->AddToDeleteList( m_pyRef ); // Don't destroy immediately. Can potentially cause problems.
}

void PyClientSideEffect::AddMeshBuilder( bp::object meshbuilder )
{
	PyMeshBuilder *pBuilder = bp::extract<PyMeshBuilder *>(meshbuilder);
	if( !pBuilder )
	{
		return;
	}

	PyMeshBuilderEntry entry;
	entry.m_pyRef = meshbuilder;
	entry.m_pMeshBuilder = pBuilder;
	m_pyMeshBuilders.AddToTail( entry );
}

void PyClientSideEffect::RemoveMeshBuilder( bp::object meshbuilder )
{
	for( int i = 0; i < m_pyMeshBuilders.Count(); i++ )
	{
		if( m_pyMeshBuilders[i].m_pyRef.ptr() == meshbuilder.ptr() )
		{
			m_pyMeshBuilders.Remove(i);
			break;
		}
	}
}

void PyClientSideEffect::ClearMeshBuilders()
{
	m_pyMeshBuilders.Purge();
}

void PyClientSideEffect::AddToEffectList( bp::object effect )
{
	if( m_pyRef.ptr() != Py_None )
		return;
	m_pyRef = effect;
	clienteffects->AddEffect(this);
}

void AddToClientEffectList( bp::object effect )
{
	PyClientSideEffect *pEffect = bp::extract<PyClientSideEffect *>(effect);
	if( !pEffect )
		return;

	pEffect->AddToEffectList(effect);
}
#endif // DISABLE_PYTHON