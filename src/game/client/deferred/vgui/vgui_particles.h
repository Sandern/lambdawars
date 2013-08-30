#ifndef VGUI_PARTICLES_H
#define VGUI_PARTICLES_H

#include "vgui_controls/controls.h"
#include "deferred/vgui/vgui_projectable.h"
#include "view_shared.h"

class CParticleCollection;

class CVGUIParticles : public CVGUIProjectable
{
	DECLARE_CLASS_SIMPLE( CVGUIParticles, CVGUIProjectable );
public:

	CVGUIParticles();
	~CVGUIParticles();

protected:
	virtual void PerformLayout();

	virtual void Paint();
	virtual void OnThink();

	virtual void ApplyConfig( KeyValues *pKV );

private:

	CParticleCollection *AddParticleSystem( const char *pszName, Vector origin = vec3_origin, QAngle angles = vec3_angle );
	void FlushAllParticles();

	CUtlVector< CParticleCollection* >m_hParticles;

	CViewSetup m_ViewSetup;
};


#endif