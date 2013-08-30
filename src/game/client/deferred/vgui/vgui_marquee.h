#ifndef VGUI_MARQUEE_H
#define VGUI_MARQUEE_H

#include "vgui_controls/controls.h"
#include "deferred/vgui/vgui_projectable.h"


class CVGUIMarquee : public CVGUIProjectable
{
	DECLARE_CLASS_SIMPLE( CVGUIMarquee, CVGUIProjectable );
public:

	CVGUIMarquee( const wchar_t *pszText = NULL, const int ispeed = 5 );
	~CVGUIMarquee();

	void SetText( const wchar_t *pszText );
	void SetFont( vgui::HFont font );
	void SetMoveSpeed( const float flspeed );

protected:
	virtual void ApplySchemeSettings( vgui::IScheme *scheme );

	virtual void Paint();
	virtual void OnThink();

	virtual void ApplyConfig( KeyValues *pKV );

private:

	wchar_t *m_pszText;
	char m_szFont[32];
	vgui::HFont m_Font;

	float m_flMoveDir;
	float m_flCurOffset;

};


#endif