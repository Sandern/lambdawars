//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "wars_mesh_builder.h"
#ifdef ENABLE_PYTHON
	#include "srcpy.h"
#endif // ENABLE_PYTHON

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef ENABLE_PYTHON
//=============================================================================
// PyMeshVertex
//=============================================================================
void PyMeshVertex::Draw( CMeshBuilder &builder )
{
	Vector drawpos = position;
	if( m_hEnt )
		drawpos += m_hEnt->GetAbsOrigin();		

	builder.Position3fv( drawpos.Base() );
	builder.Normal3fv( normal.Base() );
	builder.TexCoord2f( stage, s, t );
	builder.Color4ub( color[0], color[1], color[2], color[3] );
}

//=============================================================================
// Python mesh builder
//=============================================================================
PyMeshBuilder::PyMeshBuilder( const char *pMaterialName, MaterialPrimitiveType_t type )
	: m_nType(type), m_pMaterial(NULL)
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
	for( int i = 0; i < m_pyMeshVertices.Count(); i++ )
	{
		PyMeshVertex &vertex = m_pyMeshVertices[i];
		vertex.Draw( builder );
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
		iCount = Max(0, m_pyMeshVertices.Count()-2);
		break;
	case MATERIAL_QUADS:
		iCount = Max(0, m_pyMeshVertices.Count()-3);
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

//=============================================================================
// Python rally line mesh builder
//=============================================================================
PyMeshRallyLine::PyMeshRallyLine( const char *pMaterialName ) : PyMeshBuilder( pMaterialName, MATERIAL_QUADS ), m_hEnt1(NULL), m_hEnt2(NULL)
{

}
	

void PyMeshRallyLine::Init()
{
	Vector drawpoint1 = point1;
	Vector drawpoint2 = point2;
	if( m_hEnt1 )
		drawpoint1 += m_hEnt1->GetAbsOrigin();
	if( m_hEnt2 )
		drawpoint2 += m_hEnt2->GetAbsOrigin();

	m_fLineLength = (drawpoint1 - drawpoint2).Length();
}

void PyMeshRallyLine::Draw( double frametime )
{
	Vector drawpoint1 = point1;
	Vector drawpoint2 = point2;
	if( m_hEnt1 )
		drawpoint1 += m_hEnt1->GetAbsOrigin();
	if( m_hEnt2 )
		drawpoint2 += m_hEnt2->GetAbsOrigin();

	CMatRenderContextPtr pRenderContext( materials );

	pRenderContext->Bind( m_pMaterial );

	IMesh *pMesh = pRenderContext->GetDynamicMesh();
	
	CMeshBuilder builder;
	builder.Begin( pMesh, MATERIAL_QUADS, 1 );

	Vector dir = drawpoint1 - drawpoint2;
	VectorNormalize(dir);
        
	Vector vRight = CrossProduct( dir, Vector(0, 0, 1) );
	VectorNormalize( vRight );

	int stage = 0;

	Vector normal(0, 0, 0);

	float fLineLength = Max(1.0f, (drawpoint2 - drawpoint1).Length());

	float l = fLineLength / m_fLineLength;
	float send = m_fLineLength / Max(0.01f, texturey/textureyscale);
	float sstart = (1.0f - l) * send;

	// Setup the four points
	builder.Position3fv( ( drawpoint1 + (vRight * size) ).Base() );
	builder.Normal3fv( normal.Base() );
	builder.TexCoord2f( stage, sstart, 0.0f );
	builder.Color4ub( color[0], color[1], color[2], color[3] );
	builder.AdvanceVertex();

	builder.Position3fv( ( drawpoint2 + (vRight * size) ).Base() );
	builder.Normal3fv( normal.Base() );
	builder.TexCoord2f( stage, send, 0.0f );
	builder.Color4ub( color[0], color[1], color[2], color[3] );
	builder.AdvanceVertex();

	builder.Position3fv( ( drawpoint2 + (vRight * -size) ).Base() );
	builder.Normal3fv( normal.Base() );
	builder.TexCoord2f( stage, send, 1.0f );
	builder.Color4ub( color[0], color[1], color[2], color[3] );
	builder.AdvanceVertex();

	builder.Position3fv( ( drawpoint1 + (vRight * -size) ).Base() );
	builder.Normal3fv( normal.Base() );
	builder.TexCoord2f( stage, sstart, 1.0f );
	builder.Color4ub( color[0], color[1], color[2], color[3] );
	builder.AdvanceVertex();

	// Draw the mesh
	builder.End();
	pMesh->Draw();
}

//=============================================================================
// Python Client Side effect
//=============================================================================
PyClientSideEffect::PyClientSideEffect( const char *name )
: CClientSideEffect( name )
{
}

PyClientSideEffect::~PyClientSideEffect( void )
{
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
#endif // ENABLE_PYTHON
