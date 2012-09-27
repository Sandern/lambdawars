//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <assert.h>

#include <vgui_controls/CvarToggleCheckButton.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

vgui::Panel *StubCvarToggleCheckButton_Factory()
{
	return new CvarToggleCheckButton<ConVarRef>(NULL, NULL);
}

DECLARE_BUILD_FACTORY_CUSTOM_ALIAS( CvarToggleCheckButton<ConVarRef>, CvarToggleCheckButton, StubCvarToggleCheckButton_Factory );