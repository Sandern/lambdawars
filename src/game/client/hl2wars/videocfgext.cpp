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

#if 0
CON_COMMAND( force_dxlevel_92, "" )
{
	KeyValues *pConfigKeys = new KeyValues( "Data" );

	//struct VidMatConfigData_t data;
	//RecommendedConfig( data );

	ReadCurrentVideoConfig( pConfigKeys );

	//const MaterialSystem_Config_t &config = materials->GetCurrentConfigForVideoCard();
	//materials->GetRecommendedConfigurationInfo( 0, pConfigKeys );

	KeyValuesDumpAsDevMsg( pConfigKeys );

	Msg("Cur mat_dxlevel: %d\n", pConfigKeys->GetInt( "settings.mat_dxlevel", -1 ) );

	pConfigKeys->SetInt( "settings.mat_dxlevel", 92 );

	Msg("New mat_dxlevel: %d\n", pConfigKeys->GetInt( "settings.mat_dxlevel", -1 ) );

	//materials->
	UpdateVideoConfigConVars( pConfigKeys );
}
#endif // 0

CON_COMMAND( debug_print_dxlevel, "" )
{
	ConVarRef mat_dxlevel("mat_dxlevel");

	Msg( "nDXLevel: %d, nMaxDXLevel: %d, mat_dxlevel: %d, hdr type: %d\n", 
		g_pMaterialSystemHardwareConfig->GetDXSupportLevel(), 
		g_pMaterialSystemHardwareConfig->GetMaxDXSupportLevel(), 
		mat_dxlevel.GetInt(),
		g_pMaterialSystemHardwareConfig->GetHDRType() );
}
