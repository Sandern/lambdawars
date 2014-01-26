//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef HL2WarsVIEWPORT_H
#define HL2WarsVIEWPORT_H


#include "baseviewport.h"
#include <utlvector.h>
#include "ehandle.h"

using namespace vgui;

namespace vgui 
{
	class Panel;
}

class HL2WarsViewport : public CBaseViewport
{

private:
	DECLARE_CLASS_SIMPLE( HL2WarsViewport, CBaseViewport );

public:
	HL2WarsViewport();
	~HL2WarsViewport();

	IViewPortPanel* CreatePanelByName(const char *szPanelName);
	void			CreateDefaultPanels( void );

	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
		
	int				GetDeathMessageStartHeight( void );

	virtual void	OnThink();

	void			GetPointInScreen( Vector2D *point, Vector *world );
	virtual void	DrawMapBounderies();
	virtual void	Paint();
	virtual void PostChildPaint();

	void			DrawSelectBox();
	void			DrawRightClick();

	virtual void	UpdateCursor();

	void			UpdateSelectionBox( int iXMin, int iYMin, int iXMax, int iYMax );
	void			ClearSelectionBox();
	bool			IsSelecting() { return m_bDrawingSelectBox; }

public:
	// mouse listeners
	MESSAGE_FUNC_INT_INT( OnCursorMoved, "OnCursorMoved", x, y );
	virtual void OnCursorEntered();
	virtual void OnCursorExited();
	virtual void OnMousePressed(MouseCode code);
	virtual void OnMouseDoublePressed(MouseCode code);
	virtual void OnMouseTriplePressed(MouseCode code);
	virtual void OnMouseReleased(MouseCode code);
	virtual void OnMouseWheeled( int delta );

	bool IsMiddleMouseButtonPressed() { return m_bMiddleMouseActive; }

public:
	int					m_nMouseButtons;

private:
	bool m_bMiddleMouseActive;
	vgui::HCursor m_iDefaultMouseCursor;
	bool m_bDrawingSelectBox;
	CUtlVector< EHANDLE > m_InSelectionBox;
};


#endif // HL2WarsViewport_H
