//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: Main entry for running interpreter against server dll.
//			This is useful for using an IDE and accessing the builtin modules.
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

bool CSrcPython::InitStandAloneInterpreter()
{
	m_bPathProtected = false;

	//char originalPath[MAX_PATH];
	//V_GetCurrentDirectory( originalPath, sizeof( originalPath ) );

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
	V_strcat( pythonpath, "/python/Lib/site-packages", sizeof(pythonpath) );
	V_strcat( pythonpath, PYPATH_SEP, sizeof(pythonpath) );

	// Workaround for adding these paths
	if( filesystem->FileExists( "python/Lib", "MOD" ) == false )
	{
		filesystem->CreateDirHierarchy( "python/Lib", "MOD" );
	}
	if( filesystem->FileExists( "python/srclib", "MOD" ) == false )
	{
		filesystem->CreateDirHierarchy( "python/srclib", "MOD" );
	}

	V_strcat( pythonpath, moddir, sizeof(pythonpath) );
	V_strcat( pythonpath, "/python/Lib", sizeof(pythonpath) );
	
#ifdef OSX
	//V_strcat( pythonpath, PYPATH_SEP, sizeof(pythonpath) );
	//filesystem->RelativePathToFullPath("python/plat-darwin", "MOD", buf, sizeof(buf));
	//V_FixupPathName(buf, sizeof(buf), buf);
	V_strcat( pythonpath, "python/plat-darwin", sizeof(pythonpath) );
	
	//V_strcat( pythonpath, PYPATH_SEP, sizeof(pythonpath) );
	//filesystem->RelativePathToFullPath("python", "MOD", buf, sizeof(buf));
	//V_FixupPathName(buf, sizeof(buf), buf);
	V_strcat( pythonpath, "python", sizeof(pythonpath) );
#endif // OSX

#ifdef WIN32
	::_putenv( VarArgs( "PYTHONHOME=%s", pythonhome ) );
	::_putenv( VarArgs( "PYTHONPATH=%s", pythonpath ) );
#else
	::setenv( "PYTHONHOME", pythonhome, 1 );
    ::setenv( "PYTHONPATH", pythonpath, 1 );
#endif // WIN32

	//V_SetCurrentDirectory( originalPath );

	return true;
}

static int SrcPy_Main_Impl( int argc, wchar_t **argv )
{
	SrcPySystem()->InitStandAloneInterpreter();

	return Py_Main(argc, argv);
}

#ifdef __cplusplus
extern "C" {
#endif

#if 0
//-----------------------------------------------------------------------------
// Purpose: Compiler strips out Py_Main. Force for export to interpreter versions.
// TODO: Maybe
//-----------------------------------------------------------------------------
PyAPI_FUNC(int) SrcPy_PostInitalize()
{
#if PY_VERSION_HEX < 0x03000000
	builtins = SrcPySystem()->Import("__builtin__");
#else
	builtins = SrcPySystem()->Import("builtins");
#endif 

	srcbuiltins = SrcPySystem()->Import("srcbuiltins");

	SrcPySystem()->SysAppendPath("python\\srclib");

	// Bind common things to the builtins module, so they are available everywhere
	try 
	{
		// isclient and isserver can be used to execute specific code on client or server
		// Kind of the equivalent of CLIENT_DLL and GAME_DLL
#ifdef CLIENT_DLL
		builtins.attr("isclient") = true;
		builtins.attr("isserver") = false;
#else
		builtins.attr("isclient") = false;
		builtins.attr("isserver") = true;
#endif // CLIENT_DLL

		builtins.attr("gpGlobals") = boost::ref( gpGlobals );
		
		builtins.attr("Msg") = srcbuiltins.attr("Msg");
		builtins.attr("DevMsg") = srcbuiltins.attr("DevMsg");
		builtins.attr("PrintWarning") = srcbuiltins.attr("PrintWarning"); // Not "Warning" because Warning is already a builtin
	} 
	catch( boost::python::error_already_set & ) 
	{
		PyErr_Print();
	}

	return 1;
}
#endif // 0

PyAPI_FUNC(int) SrcPy_Main( int argc, wchar_t **argv )
{
	return SrcPy_Main_Impl( argc, argv );
}
#ifdef __cplusplus
}
#endif