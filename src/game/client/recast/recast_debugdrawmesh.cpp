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

void DebugDrawMesh::begin(duDebugDrawPrimitives prim, float size)
{
#if 0
	switch (prim)
	{
		case DU_DRAW_POINTS:
			glPointSize(size);
			glBegin(GL_POINTS);
			break;
		case DU_DRAW_LINES:
			glLineWidth(size);
			glBegin(GL_LINES);
			break;
		case DU_DRAW_TRIS:
			glBegin(GL_TRIANGLES);
			break;
		case DU_DRAW_QUADS:
			glBegin(GL_QUADS);
			break;
	};
#endif // 0
	CMatRenderContextPtr pRenderContext( materials );
	m_pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, m_mat );

	switch( prim )
	{
		case DU_DRAW_POINTS:
			m_meshBuilder.Begin( m_pMesh, MATERIAL_POINTS, 32000 );
			break;
		case DU_DRAW_LINES:
			m_meshBuilder.Begin( m_pMesh, MATERIAL_LINES, 32000 );
			break;
		case DU_DRAW_TRIS:
			m_meshBuilder.Begin( m_pMesh, MATERIAL_TRIANGLES, 32000 );
			break;
		case DU_DRAW_QUADS:
			m_meshBuilder.Begin( m_pMesh, MATERIAL_QUADS, 32000 );
			break;
	};
}

void DebugDrawMesh::vertex(const float* pos, unsigned int color)
{
	m_meshBuilder.Color4Packed( color );
	m_meshBuilder.Position3f( pos[0], pos[2], pos[1] );
	m_meshBuilder.AdvanceVertex();
	//glColor4ubv((GLubyte*)&color);
	//glVertex3fv(pos);
}

void DebugDrawMesh::vertex(const float x, const float y, const float z, unsigned int color)
{
	m_meshBuilder.Color4Packed( color );
	m_meshBuilder.Position3f( x, z, y );
	m_meshBuilder.AdvanceVertex();
	//glColor4ubv((GLubyte*)&color);
	//glVertex3f(x,y,z);
}

void DebugDrawMesh::vertex(const float* pos, unsigned int color, const float* uv)
{
	m_meshBuilder.Color4Packed( color );
	m_meshBuilder.TexCoord2fv( 0, uv );
	m_meshBuilder.Position3f( pos[0], pos[2], pos[1] );
	m_meshBuilder.AdvanceVertex();

	//glColor4ubv((GLubyte*)&color);
	//glTexCoord2fv(uv);
	//glVertex3fv(pos);
}

void DebugDrawMesh::vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v)
{
	m_meshBuilder.Color4Packed( color );
	m_meshBuilder.TexCoord2f( 0, u, v );
	m_meshBuilder.Position3f( x, z, y );
	m_meshBuilder.AdvanceVertex();

	//glColor4ubv((GLubyte*)&color);
	//glTexCoord2f(u,v);
	//glVertex3f(x,y,z);
}

void DebugDrawMesh::end()
{
	//glEnd();
	//glLineWidth(1.0f);
	//glPointSize(1.0f);
	m_meshBuilder.End();
	m_pMesh->Draw();
}
