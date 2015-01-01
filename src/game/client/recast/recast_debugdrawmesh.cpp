//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "recast_debugdrawmesh.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DebugDrawMesh::DebugDrawMesh() : m_bBindTexture(false)
{
	m_mat.Init( "editor/recast_debug", TEXTURE_GROUP_OTHER );
	Assert( m_mat.IsValid() );
}

DebugDrawMesh::~DebugDrawMesh()
{
	m_mat.Shutdown();
}

void DebugDrawMesh::depthMask(bool state)
{
	//glDepthMask(state ? GL_TRUE : GL_FALSE);
}

void DebugDrawMesh::texture(bool state)
{
	m_bBindTexture = state;
}

void DebugDrawMesh::begin(duDebugDrawPrimitives prim, int num, float size)
{
	CMatRenderContextPtr pRenderContext( materials );
	m_pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, m_mat );

	if( num == 0 )
	{
		int nMaxVerts, nMaxIndices;
		pRenderContext->GetMaxToRender( m_pMesh, false, &nMaxVerts, &nMaxIndices );

		switch( prim )
		{
			case DU_DRAW_POINTS:
				num = nMaxVerts;
				break;
			case DU_DRAW_LINES:
				num = nMaxVerts / 2;
				break;
			case DU_DRAW_TRIS:
				num = nMaxVerts / 3;
				break;
			case DU_DRAW_QUADS:
				num = nMaxVerts / 4;
				break;
		};
	}

	//Msg("Rendering prim %d with max primatives %d\n", prim, num);
	switch( prim )
	{
		case DU_DRAW_POINTS:
			m_meshBuilder.Begin( m_pMesh, MATERIAL_POINTS, num );
			break;
		case DU_DRAW_LINES:
			m_meshBuilder.Begin( m_pMesh, MATERIAL_LINES, num );
			break;
		case DU_DRAW_TRIS:
			m_meshBuilder.Begin( m_pMesh, MATERIAL_TRIANGLES, num );
			break;
		case DU_DRAW_QUADS:
			m_meshBuilder.Begin( m_pMesh, MATERIAL_QUADS, num );
			break;
	};
}

void DebugDrawMesh::vertex(const float* pos, unsigned int color)
{
	m_meshBuilder.Color4Packed( color );
	m_meshBuilder.Position3f( pos[0], pos[2], pos[1] );
	m_meshBuilder.AdvanceVertex();
}

void DebugDrawMesh::vertex(const float x, const float y, const float z, unsigned int color)
{
	m_meshBuilder.Color4Packed( color );
	m_meshBuilder.Position3f( x, z, y );
	m_meshBuilder.AdvanceVertex();
}

void DebugDrawMesh::vertex(const float* pos, unsigned int color, const float* uv)
{
	m_meshBuilder.Color4Packed( color );
	m_meshBuilder.TexCoord2fv( 0, uv );
	m_meshBuilder.Position3f( pos[0], pos[2], pos[1] );
	m_meshBuilder.AdvanceVertex();
}

void DebugDrawMesh::vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v)
{
	m_meshBuilder.Color4Packed( color );
	m_meshBuilder.TexCoord2f( 0, u, v );
	m_meshBuilder.Position3f( x, z, y );
	m_meshBuilder.AdvanceVertex();
}

void DebugDrawMesh::end()
{
	m_meshBuilder.End();
	m_pMesh->Draw();
}
