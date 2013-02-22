//====== Copyright © 2013 Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"	
#include "wars_vgui_screen.h"
#include <vgui_controls/panel.h>
#include "CollisionUtils.h"
#include "VGUIMatSurface/IMatSystemSurface.h"
#include "view.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef ENABLE_PYTHON

WarsVGUIScreen::WarsVGUIScreen() : m_pPanel(NULL)
{
	m_flWidth = 128.0f;
	m_flHeight = 128.0f;
	m_vOrigin = vec3_origin;
	m_vAngles = vec3_angle;

	m_WriteZMaterial.Init( "engine/writez", TEXTURE_GROUP_VGUI );
	m_OverlayMaterial.Init( m_WriteZMaterial );
}

//-----------------------------------------------------------------------------
// Computes control points of the quad describing the screen
//-----------------------------------------------------------------------------
void WarsVGUIScreen::ComputeEdges( Vector *pUpperLeft, Vector *pUpperRight, Vector *pLowerLeft )
{
	Vector vecOrigin = m_vOrigin;
	Vector xaxis, yaxis;
	AngleVectors( m_vAngles, &xaxis, &yaxis, NULL );

	// NOTE: Have to multiply by -1 here because yaxis goes out the -y axis in AngleVectors actually...
	yaxis *= -1.0f;

	VectorCopy( vecOrigin, *pLowerLeft );
	VectorMA( vecOrigin, m_flHeight, yaxis, *pUpperLeft );
	VectorMA( *pUpperLeft, m_flWidth, xaxis, *pUpperRight );
}


//-----------------------------------------------------------------------------
// Return intersection point of ray with screen in barycentric coords
//-----------------------------------------------------------------------------
bool WarsVGUIScreen::IntersectWithRay( const Ray_t &ray, float *u, float *v, float *t )
{
	// Perform a raycast to see where in barycentric coordinates the ray hits
	// the viewscreen; if it doesn't hit it, you're not in the mode
	Vector origin, upt, vpt;
	ComputeEdges( &origin, &upt, &vpt );
	return ComputeIntersectionBarycentricCoordinates( ray, origin, upt, vpt, *u, *v, t );
}


//-----------------------------------------------------------------------------
// Is the vgui screen backfacing?
//-----------------------------------------------------------------------------
bool WarsVGUIScreen::IsBackfacing( const Vector &viewOrigin )
{
#if 0
	// Compute a ray from camera to center of the screen..
	Vector cameraToScreen;
	VectorSubtract( (m_hEntity ? m_hEntity->GetAbsOrigin() + m_vOrigin : m_vOrigin), viewOrigin, cameraToScreen );

	// Figure out the face normal
	Vector zaxis;
	bla->GetVectors( NULL, NULL, &zaxis );

	// The actual backface cull
	return (DotProduct( zaxis, cameraToScreen ) > 0.0f);
#else
	return false;
#endif // 0
}


//-----------------------------------------------------------------------------
//  Computes the panel center to world transform
//-----------------------------------------------------------------------------
void WarsVGUIScreen::ComputePanelToWorld()
{
	// The origin is at the upper-left corner of the screen
	Vector vecOrigin, vecUR, vecLL;
	ComputeEdges( &vecOrigin, &vecUR, &vecLL );
	m_PanelToWorld.SetupMatrixOrgAngles( vecOrigin, m_vAngles );
}

//-----------------------------------------------------------------------------
// a pass to set the z buffer...
//-----------------------------------------------------------------------------
void WarsVGUIScreen::DrawScreenOverlay()
{
	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->MatrixMode( MATERIAL_MODEL );
	pRenderContext->PushMatrix();
	pRenderContext->LoadMatrix( m_PanelToWorld );

	unsigned char pColor[4] = {255, 255, 255, 255};

	CMeshBuilder meshBuilder;
	IMesh *pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, m_OverlayMaterial );
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Position3f( 0.0f, 0.0f, 0 );
	meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
	meshBuilder.Color4ubv( pColor );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( m_flWidth, 0.0f, 0 );
	meshBuilder.TexCoord2f( 0, 1.0f, 0.0f );
	meshBuilder.Color4ubv( pColor );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( m_flWidth, -m_flHeight, 0 );
	meshBuilder.TexCoord2f( 0, 1.0f, 1.0f );
	meshBuilder.Color4ubv( pColor );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( 0.0f, -m_flHeight, 0 );
	meshBuilder.TexCoord2f( 0, 0.0f, 1.0f );
	meshBuilder.Color4ubv( pColor );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();

	pRenderContext->PopMatrix();
}


void WarsVGUIScreen::SetPanel( boost::python::object panel )
{
	if( panel.ptr() == Py_None )
	{
		m_pPanel = NULL;
		m_pyPanel = boost::python::object();
		return;
	}

	try {
		m_pPanel = boost::python::extract<vgui::Panel *>(panel);
		m_pyPanel = panel;
	} catch(boost::python::error_already_set &) {
		PyErr_Print();
		PyErr_Clear();
		m_pPanel = NULL;
		m_pyPanel = boost::python::object();
		return;
	}
}

void WarsVGUIScreen::SetWorldSize( float width, float height )
{
	m_flWidth = width;
	m_flHeight = height;
}

void WarsVGUIScreen::SetOrigin( const Vector &vOrigin )
{
	m_vOrigin = vOrigin;
}

void WarsVGUIScreen::SetAngles( const QAngle &vAngles )
{
	m_vAngles = vAngles;
}

void WarsVGUIScreen::Draw()
{
	if( !m_pPanel )
		return;

	// Pixel size
	int x, y;
	m_pPanel->GetBounds( x, y, m_nPixelWidth, m_nPixelHeight );

	ComputePanelToWorld();

	//CMatRenderContextPtr pRenderContext( materials );
	//pRenderContext->OverrideDepthEnable( true, false );

	g_pMatSystemSurface->DrawPanelIn3DSpace( m_pPanel->GetVPanel(), m_PanelToWorld, 
		m_nPixelWidth, m_nPixelHeight, m_flWidth, m_flHeight );

	//DrawScreenOverlay();
}

#endif // ENABLE_PYTHON
