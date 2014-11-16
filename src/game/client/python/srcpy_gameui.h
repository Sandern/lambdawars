//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
//
// $NoKeywords: $
//=============================================================================//

#ifndef SRCPY_GAMEUI_H
#define SRCPY_GAMEUI_H

#ifdef _WIN32
#pragma once
#endif

#include "gameui/wars/basemodpanel.h"

// Temporary function until everything is ported over to the new html based menu
void PyGameUICommand( const char *command );
void PyGameUIOpenWindow( BaseModUI::WINDOW_TYPE windowtype, bool hideprevious = true, KeyValues *parameters = NULL );

void OpenGammaDialog( vgui::VPANEL parent );

boost::python::object GetMainMenu();

#endif // SRCPY_GAMEUI_H