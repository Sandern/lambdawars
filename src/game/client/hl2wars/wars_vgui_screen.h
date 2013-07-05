//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
// TODO: Move into the Python folder
//
//=============================================================================//

#ifndef WARS_VGUI_SCREEN_H
#define WARS_VGUI_SCREEN_H

#ifdef _WIN32
#pragma once
#endif

#ifdef ENABLE_PYTHON

class WarsVGUIScreen
{
public:
	WarsVGUIScreen();

	void SetPanel( boost::python::object panel );
	void SetWorldSize( float width, float height );

	void SetOrigin( const Vector &vOrigin );
	void SetAngles( const QAngle &vAngles );

	float GetWide() { return m_flWidth; }
	float GetHeight() { return m_flHeight; }

	bp::object GetPanel() { return m_pyPanel; }

	// Is the screen backfaced given a view position?
	bool IsBackfacing( const Vector &viewOrigin );

	// Return intersection point of ray with screen in barycentric coords
	bool IntersectWithRay( const Ray_t &ray, float *u, float *v, float *t );

	virtual void Draw();

private:
	//  Computes the panel to world transform
	void ComputePanelToWorld();

	// Computes control points of the quad describing the screen
	void ComputeEdges( Vector *pUpperLeft, Vector *pUpperRight, Vector *pLowerLeft );

	// Writes the z buffer
	void DrawScreenOverlay();

private:
	vgui::Panel *m_pPanel;
	boost::python::object m_pyPanel;

	VMatrix	m_PanelToWorld;
	float m_flWidth;
	float m_flHeight;
	int m_nPixelWidth; 
	int m_nPixelHeight;

	Vector m_vOrigin;
	QAngle m_vAngles;

	CMaterialReference	m_WriteZMaterial;
	CMaterialReference	m_OverlayMaterial;
};

#endif // ENABLE_PYTHON

#endif // WARS_VGUI_SCREEN_H