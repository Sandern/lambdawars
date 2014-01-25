#ifndef WARS_MODEL_PANEL_H
#define WARS_MODEL_PANEL_H

#ifdef _WIN32
#pragma once
#endif

#include "game_controls/basemodel_panel.h"

class CStudioHdr;

//-----------------------------------------------------------------------------
// Base class for all Alien Swarm model panels
//-----------------------------------------------------------------------------
class CWars_Model_Panel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CWars_Model_Panel, vgui::EditablePanel );
public:
	CWars_Model_Panel( vgui::Panel *parent, const char *name );
	virtual ~CWars_Model_Panel();

	virtual void Paint();

#if 0
	// function to set up scene sets
	void SetupCustomLights( Color cAmbient, Color cKey, float fKeyBoost, Color cRim, float fRimBoost );

	// Lights
	virtual void SetAmbientCube( Color &cAmbient );
	virtual void AddLight( LightDesc_t &newlight, const char *attachlight );
	virtual void AddLight( LightDesc_t &newlight, matrix3x4_t &lightToWorld );
	virtual void ClearLights( );
	LightDesc_t GetLight( int i );
	matrix3x4_t GetLightToWorld( int i );

	void SetBodygroup( int iGroup, int iValue );
	int FindBodygroupByName( const char *name );
	int GetNumBodyGroups();

	void SetModelAnglesAndPosition( const QAngle &angRot, const Vector &vecPos );
	void LookAtBounds( const Vector &vecBoundsMin, const Vector &vecBoundsMax );

	// TODO: Add support for flex weights/rules to this panel


protected:
	bool m_bShouldPaint;

	Vector m_vecModelPos;
	QAngle m_angModelRot;

private:
	CStudioHdr* m_pStudioHdr;
#endif // 0
};


#endif // WARS_MODEL_PANEL_H
