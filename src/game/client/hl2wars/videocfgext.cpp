#include "cbase.h"
#include "videocfgext.h"
#include "filesystem.h"

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