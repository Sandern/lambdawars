#ifndef VGUI_PROJECTABLE_H
#define VGUI_PROJECTABLE_H

#include "vgui_controls/controls.h"
#include "vgui_controls/panel.h"

class ITexture;

class CVGUIProjectable : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CVGUIProjectable, vgui::Panel );
public:

	CVGUIProjectable();
	~CVGUIProjectable();

	void LoadProjectableConfig( KeyValues *pKV );
	void SetControllableName( const char *name );
	void SetParentName( const char *name );

	virtual void DrawSelfToRT( ITexture *pTarget, bool bClear = true );

protected:
	virtual void PerformLayout();
	virtual void ApplySchemeSettings( vgui::IScheme *scheme );

	virtual void ApplyConfig( KeyValues *pKV );

private:
	char *m_pszControllableName;
	char *m_pszParentName;

	KeyValues *m_pConfig;
};


#endif