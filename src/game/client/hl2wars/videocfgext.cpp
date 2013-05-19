//====== Copyright © 2013 Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "videocfgext.h"
#include "filesystem.h"

#include "videocfg/videocfg.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

extern ConVar deferred_lighting_enabled;

void ReadVideoCfgExt()
{
	KeyValues *pConfig = new KeyValues("VideoExtConfig");
	if( !pConfig )
		return;

	if( pConfig->LoadFromFile( filesystem, "cfg/videoext.txt", "MOD" ) || pConfig->LoadFromFile( filesystem, "cfg/videoext_default.txt", "MOD" ) )
	{
		//deferred_lighting_enabled.SetValue(
		//	pConfig->GetBool( "setting.deferred_lighting", deferred_lighting_enabled.GetBool() ) );
	}

	pConfig->deleteThis();
}

void SaveVideoCfgExt()
{
	KeyValues *pConfig = new KeyValues("VideoExtConfig");
	if( !pConfig )
		return;

	//pConfig->SetBool( "setting.deferred_lighting", deferred_lighting_enabled.GetBool() );

	pConfig->SaveToFile( filesystem, "cfg/videoext.txt", "MOD");

	pConfig->deleteThis();
}

CON_COMMAND( debug_print_dxlevel, "" )
{
	ConVarRef mat_dxlevel("mat_dxlevel");

	Msg( "nDXLevel: %d, nMaxDXLevel: %d, mat_dxlevel: %d, hdr type: %d, HasFastVertexTextures: %d\n", 
		g_pMaterialSystemHardwareConfig->GetDXSupportLevel(), 
		g_pMaterialSystemHardwareConfig->GetMaxDXSupportLevel(), 
		mat_dxlevel.GetInt(),
		g_pMaterialSystemHardwareConfig->GetHDRType(),
		g_pMaterialSystemHardwareConfig->HasFastVertexTextures()
		);
}
