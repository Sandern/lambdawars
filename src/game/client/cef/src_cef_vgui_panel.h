//====== Copyright 20xx, Sander Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef SRC_CEF_VGUI_PANEL_H
#define SRC_CEF_VGUI_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>
#include "materialsystem/MaterialSystemUtil.h"

class SrcCefBrowser;
class CCefTextureGenerator;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class SrcCefVGUIPanel : public vgui::Panel 
{
public:
	DECLARE_CLASS_SIMPLE(SrcCefVGUIPanel, vgui::Panel);

	SrcCefVGUIPanel( SrcCefBrowser *pController, vgui::Panel *pParent );
	~SrcCefVGUIPanel();

	virtual void ResizeTexture( int width, int height );
	virtual void MarkTextureDirty( int dirtyx, int dirtyy, int dirtyw, int dirtyh );

	virtual void Paint();
	virtual void OnSizeChanged(int newWide, int newTall);
	virtual void PerformLayout();

	int	GetEventFlags();

	virtual void OnCursorEntered();
	virtual void OnCursorExited();
	virtual void OnCursorMoved(int x,int y);
	virtual void OnMousePressed(vgui::MouseCode code);
	virtual void OnMouseDoublePressed(vgui::MouseCode code);
	virtual void OnMouseReleased(vgui::MouseCode code);
	virtual void OnMouseWheeled(int delta);

	virtual void OnKeyCodePressed(vgui::KeyCode code);
	virtual void OnKeyCodeTyped(vgui::KeyCode code);
	virtual void OnKeyTyped(wchar_t unichar);
	virtual void OnKeyCodeReleased(vgui::KeyCode code);

	virtual vgui::HCursor GetCursor();

private:
	int m_iMouseX, m_iMouseY;
	int m_iEventFlags;
	SrcCefBrowser *m_pBrowser;

	// Texture variables
	bool m_bSizeChanged;
	CTextureReference	m_RenderBuffer;
	CCefTextureGenerator *m_pTextureRegen;
	CMaterialReference m_MatRef;
	char m_MatWebViewName[MAX_PATH];
	char m_TextureWebViewName[MAX_PATH];

	int m_iTextureID;
	int m_iWVWide, m_iWVTall;
	int m_iTexWide, m_iTexTall;
	Color m_Color;
	float m_fTexS1, m_fTexT1;

	Rect_t m_DirtyArea;
};

#endif // SRC_CEF_VGUI_PANEL_H