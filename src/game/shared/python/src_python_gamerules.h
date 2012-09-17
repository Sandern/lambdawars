//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose: The TF Game rules object
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef SRCPYTHON_GAMERULES_H
#define SRCPYTHON_GAMERULES_H

#ifdef _WIN32
#pragma once
#endif

void					PyInstallGameRules( boost::python::object gamerules );
boost::python::object	PyGameRules();
void					ClearPyGameRules();

#endif // SRCPYTHON_GAMERULES_H