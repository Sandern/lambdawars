//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: Main entry for running interpreter against server dll.
//			This is useful for using an IDE and accessing the builtin modules.
//
// Caveats:
//  - Even though we initialize the filesystem here, most modules will not use 
//    Source filesystem (or a lot of stuff needs to be rewritten). Only the import
//	  system and select number of modules has been modified for this.
//	  For this reason, the standalone interpreter changes Python/Lib to Python/LibDev
//	  and expects the extracted Lib folder in there.
//
// TODO:
//	- In thirdparty/python/Python/pythonrun.c, stdout/stderr should be initialized,
//	  but only for when running as a Python interpreter (otherwise causes issues on
//	  some machines)
//  - (Optional) Split thirdparty/python/PCBuild/python and pythonw projects.
//    Need modifications to load the embedded Python from the server/client dll.
//  - Test with different IDEs. Figure out what's needed to make it work properly.
//	  Mainly Visual Studio Python Tools and PyCharm.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "srcpy_standalone.h"
#include "srcpy.h"
#include <filesystem.h>
#include <filesystem_init.h>

#include <Windows.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define PY_MAX_PATH 2048

#ifndef CLIENT_DLL
#define VarArgs UTIL_VarArgs
#endif // CLIENT_DLL

static int s_iStandAloneInterpreter = 0;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSrcPython::InitStandAloneInterpreter()
{
	s_iStandAloneInterpreter = 1;
	m_bPythonRunning = true;
	m_bPathProtected = false;

	// Temporary change working directory for initalizing the filesystem
	char originalPath[MAX_PATH];
	V_GetCurrentDirectory( originalPath, sizeof( originalPath ) );

	// Hook up the filesystem
	TCHAR fullPath[MAX_PATH];
	TCHAR driveLetter[3];
	TCHAR directory[MAX_PATH];
	GetModuleFileName(NULL, fullPath, MAX_PATH);
	_splitpath(fullPath, driveLetter, directory, NULL, NULL);

	V_SetCurrentDirectory( VarArgs( "%s%s//..//..", driveLetter, directory ) );

	CSysModule *h = Sys_LoadModule( "filesystem_stdio.dll" );

	CreateInterfaceFn fileSystemFactory = Sys_GetFactory( h );
	if( !fileSystemFactory )
		return 0;

	if ( (filesystem = (IFileSystem *)fileSystemFactory(FILESYSTEM_INTERFACE_VERSION,NULL)) == NULL )
		return 0;

	char szFullModPath[MAX_PATH];
	_fullpath( szFullModPath, "lambdawarsdev", sizeof( szFullModPath ) );

	// Init filesystem
	CFSSearchPathsInit initInfo;
	initInfo.m_pFileSystem = filesystem;
	initInfo.m_pDirectoryName = szFullModPath;
	FileSystem_LoadSearchPaths( initInfo );

	/* Copied from srcpy.cpp */
	// Change working directory	
	char moddir[MAX_PATH];
	filesystem->RelativePathToFullPath( ".", "MOD", moddir, sizeof( moddir ) );
	V_FixupPathName( moddir, sizeof( moddir ), moddir );	
	V_SetCurrentDirectory( moddir );

	filesystem->AddVPKFile( "python\\pythonlib_dir.vpk" );

	char pythonpath[PY_MAX_PATH];
	pythonpath[0] = '\0';
	char pythonhome[PY_MAX_PATH];
	pythonhome[0] = '\0';
	
#ifdef WIN32
#define PYPATH_SEP ";" // Semicolon on Windows
#else
#define PYPATH_SEP ":" // Colon on Unix
#endif // WIN32

	// Set PYTHONHOME
	V_strcat( pythonhome, "python", sizeof(pythonhome) );
	
	// Set PYTHONPATH
#ifdef WIN32
#ifdef CLIENT_DLL
	V_strcat( pythonpath, "python/ClientDLLs", sizeof(pythonpath) );
#else
	
	V_strcat( pythonpath, moddir, sizeof(pythonpath) );
	V_strcat( pythonpath, "/python/DLLs", sizeof(pythonpath) );
#endif // CLIENT_DLL
	V_strcat( pythonpath, PYPATH_SEP, sizeof(pythonpath) );
#endif // WIN32

	V_strcat( pythonpath, moddir, sizeof(pythonpath) );
	V_strcat( pythonpath, "/python", sizeof(pythonpath) );
	V_strcat( pythonpath, PYPATH_SEP, sizeof(pythonpath) );

	V_strcat( pythonpath, moddir, sizeof(pythonpath) );
	V_strcat( pythonpath, "/python/LibDev/site-packages", sizeof(pythonpath) );
	V_strcat( pythonpath, PYPATH_SEP, sizeof(pythonpath) );

	// Workaround for adding these paths
	if( filesystem->FileExists( "python/LibDev", "MOD" ) == false )
	{
		filesystem->CreateDirHierarchy( "python/LibDev", "MOD" );
	}

	V_strcat( pythonpath, moddir, sizeof(pythonpath) );
	V_strcat( pythonpath, "/python/LibDev", sizeof(pythonpath) );
	
#ifdef OSX
	V_strcat( pythonpath, "python/plat-darwin", sizeof(pythonpath) );
	
	V_strcat( pythonpath, "python", sizeof(pythonpath) );
#endif // OSX

#ifdef WIN32
	::_putenv( VarArgs( "PYTHONHOME=%s", pythonhome ) );
	::_putenv( VarArgs( "PYTHONPATH=%s", pythonpath ) );
#else
	::setenv( "PYTHONHOME", pythonhome, 1 );
    ::setenv( "PYTHONPATH", pythonpath, 1 );
#endif // WIN32

	// Change back working directory
	V_SetCurrentDirectory( originalPath );

	return true;
}

static int SrcPy_Main_Impl( int argc, wchar_t **argv )
{
	SrcPySystem()->InitStandAloneInterpreter();

	int ret = Py_Main(argc, argv);

	SrcPySystem()->PostShutdownInterpreter( true );

	return ret;
}

#ifdef __cplusplus
extern "C" {
#endif

//-----------------------------------------------------------------------------
// Purpose: Used by thirdparty/Python/Python/pythonrun.c to determine if stdio
//			should be initialized. Skipped for Game, because it will be
//			redirected. Also seems to give problems on machines of some users.
//-----------------------------------------------------------------------------
PyAPI_FUNC(int) SrcPy_IsStandAloneInterpreter()
{
	return s_iStandAloneInterpreter;
}

//-----------------------------------------------------------------------------
// Purpose: To be called from thirdparty/Python/Modules/main.c, 
// after Py_Initialize
//-----------------------------------------------------------------------------
PyAPI_FUNC(int) SrcPy_Main_PostInitalize()
{
	if( !SrcPySystem()->PostInitInterpreter( true ) )
		return 0;
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: To be called from thirdparty/Python/Modules/main.c, 
// before Py_Finalize
// To be called from thirdparty/Python/Modules/pythonrun.c, 
// before Py_Exit
//-----------------------------------------------------------------------------
PyAPI_FUNC(int) SrcPy_Main_PreFinalize()
{
	if( !SrcPySystem()->PreShutdownInterpreter( true ) )
		return 0;
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: To be called from thirdparty/Python/Modules/pythonrun.c, 
// after Py_Exit
//-----------------------------------------------------------------------------
PyAPI_FUNC(int) SrcPy_Main_PostFinalize()
{
	if( !SrcPySystem()->PostShutdownInterpreter( true ) )
		return 0;
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: To be called from thirdparty/Python/Modules/python.c, 
// Initializes the embedded interpreter.
//-----------------------------------------------------------------------------
PyAPI_FUNC(int) SrcPy_Main( int argc, wchar_t **argv )
{
	return SrcPy_Main_Impl( argc, argv );
}
#ifdef __cplusplus
}
#endif