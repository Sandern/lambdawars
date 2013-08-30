#include "cbase.h"
#include "deferred/deferred_shared_common.h"


CSysModule *__g_pDeferredShaderModule = NULL;
static IDeferredExtension *__g_defExt = NULL;
IDeferredExtension *GetDeferredExt()
{
	return __g_defExt;
}

bool ConnectDeferredExt()
{
	char modulePath[MAX_PATH*4];
	Q_snprintf( modulePath, sizeof( modulePath ), "%s/bin/game_shader_dx9.dll\0", engine->GetGameDirectory() );
	__g_pDeferredShaderModule = Sys_LoadModule( modulePath );

	if ( __g_pDeferredShaderModule )
	{
		CreateInterfaceFn shaderDeferredDLLFactory = Sys_GetFactory( __g_pDeferredShaderModule );
		__g_defExt = shaderDeferredDLLFactory ? ((IDeferredExtension *) shaderDeferredDLLFactory( DEFERRED_EXTENSION_VERSION, NULL )) : NULL;

		if ( !__g_defExt )
			Warning( "Unable to pull IDeferredExtension interface.\n" );
	}
	else
		Warning( "Cannot load game_shader_dx9.dll from %s!\n", modulePath );

	return __g_defExt != NULL;
}

void ShutdownDeferredExt()
{
	if ( !__g_defExt )
		return;

	delete [] __g_defExt->CommitLightData_Common( NULL, 0, 0, 0, 0, 0 );

	__g_defExt = NULL;
}