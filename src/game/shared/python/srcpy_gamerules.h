//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: Manages Python implemented gamerules.
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef SRCPY_GAMERULES_H
#define SRCPY_GAMERULES_H

#ifdef _WIN32
#pragma once
#endif

#if defined( HL2WARS_DLL )
#define PYGAMERULES "CHL2WarsGameRules"
#elif defined( HL2MP )
#define PYGAMERULES "CHL2MPRules"
#elif defined( HL2_DLL )
#define PYGAMERULES "CHalfLife2"
#else
#error "Must define a proper gamerules base"
#endif



void					PyInstallGameRules( boost::python::object gamerules );
boost::python::object	PyGameRules();
void					ClearPyGameRules();

#endif // SRCPY_GAMERULES_H