//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef WARS_MOUNT_SYSTEM_H
#define WARS_MOUNT_SYSTEM_H

#ifdef _WIN32
#pragma once
#endif

enum AppStatus {
	AS_NOT_AVAILABLE = 0,
	AS_AVAILABLE_REQUIRES_EXTRACTION,
	AS_AVAILABLE,
};

enum MountApps {
#ifdef HL2WARS_ASW_DLL
	APP_L4D1 = 0,
	APP_L4D2,
	APP_PORTAL2,
	APP_DOTA,
	APP_CSGO,
	APP_DEARESTHER,
#else
	APP_EP2 = 0,
	APP_EP1,
	APP_TF2,
	APP_PORTAL,
	APP_CSS,
	APP_HL1,
	APP_DODS,
#endif // 0

	NUM_APPS,
};

bool HasApp( MountApps app );
AppStatus GetAppStatus( MountApps app );
void MountExtraContent();
void PostInitExtraContent();

#endif // WARS_MOUNT_SYSTEM_H