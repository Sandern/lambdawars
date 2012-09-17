//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef WARS_WEAPON_SHARED_H
#define WARS_WEAPON_SHARED_H

#ifdef _WIN32
#pragma once
#endif


// Shared header file for weapons
#if defined( CLIENT_DLL )
	#define CWarsWeapon C_WarsWeapon
#endif

#endif // WARS_WEAPON_SHARED_H