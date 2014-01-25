//========= Copyright 1996-2012, Valve Corporation, Khrona LLC, Nicolas and Biohazard All rights reserved. ============//
//
// Purpose: Drawing a windowless web-browser framework on HUD (VGUI) panel.
//
// $NoKeywords: $
//=====================================================================================================================//

#ifndef WEB_HUD_FULL_H
#define WEB_HUD_FULL_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include <vgui_controls/Panel.h>
#include <Awesomium/WebCore.h>

namespace vgui
{
	class IScheme;
};

class SrcWebViewSurface;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CWebHudFull : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CWebHudFull, vgui::Panel );
public:
	CWebHudFull( const char *pElementName );
	~CWebHudFull( void );

protected:
	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void	Paint();
	virtual void    OnThink();

	void SetWebViewSize( int newWide, int newTall );
	void RebuildWebView();

	bool ShouldDraw( void );

	// Rendering
	void RenderWebPanel( void );

	// Health
	void HealthParse( void );
	void Health( void );

	// Awesomium API
    Awesomium::WebView* webView;

	SrcWebViewSurface *m_pWebViewSurface;

	int m_iWVWide, m_iWVTall;
	bool m_bCustomViewSize;
	bool m_bRebuildPending;

	Color m_Color;
	int m_iTextureID;

private:
	// Health
	int		m_iHealth;

	int viewWidth, viewHeight;
};

#endif // WEB_HUD_FULL_H
