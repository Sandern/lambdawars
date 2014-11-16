//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "srcpy_gameui.h"

#include "gameui/wars/basemodpanel.h"
#include "gameui/wars/basemodframe.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern boost::python::object s_ref_ui_basemodpanel;

//-----------------------------------------------------------------------------
// Temporary function until everything is ported over to the new html based
// menu.
//-----------------------------------------------------------------------------
void PyGameUICommand( const char *command )
{
	BaseModUI::WINDOW_TYPE type = BaseModUI::CBaseModPanel::GetSingleton().GetActiveWindowType();
	BaseModUI::CBaseModFrame *pActivePanel = BaseModUI::CBaseModPanel::GetSingleton().GetWindow( type );
	if( pActivePanel )
		pActivePanel->OnCommand( command );
}

void PyGameUIOpenWindow( BaseModUI::WINDOW_TYPE windowType, bool hidePrevious, KeyValues *pParameters )
{
	BaseModUI::CBaseModPanel &baseModPanel = BaseModUI::CBaseModPanel::GetSingleton();
	baseModPanel.OpenWindow( windowType, NULL, hidePrevious, pParameters );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
boost::python::object GetMainMenu()
{
	return s_ref_ui_basemodpanel;
}