//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "srcpy.h"
#include <filesystem.h>
#include "icommandline.h"
#include "srcpy_usermessage.h"
#include "srcpy_gamerules.h"
#include "srcpy_entities.h"
#include "srcpy_networkvar.h"
#include "gamestringpool.h"

#ifdef CLIENT_DLL
	#include "networkstringtable_clientdll.h"
	#include "srcpy_materials.h"
	#include "gameui/wars/basemodpanel.h"
#else
	#include "networkstringtable_gamedll.h"
#endif // CLIENT_DLL

#ifdef WIN32
#include <winlite.h>
#endif // WIN32

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef CLIENT_DLL
#define VarArgs UTIL_VarArgs
#endif // CLIENT_DLL

// Shorter alias
namespace bp = boost::python;

#ifdef CLIENT_DLL
	extern void DestroyPyPanels();
#endif // CLIENT_DLL

#if 0
// Stubs for Python
const char *Py_GetBuildInfo(void) { return "SourcePy"; }
const char *_Py_hgversion(void) { return "1"; }
const char *_Py_hgidentifier(void) { return "srcpy"; }
#endif // 0

#ifdef WIN32
extern "C" 
{
	char dllVersionBuffer[16] = ""; // a private buffer
	HMODULE PyWin_DLLhModule = NULL;
	const char *PyWin_DLLVersionString = dllVersionBuffer;
}
#endif // WIN32

// For debugging
ConVar g_debug_python( "g_debug_python", "0", FCVAR_REPLICATED );

extern ConVar g_debug_pynetworkvar;

const Color g_PythonColor( 0, 255, 0, 255 );

// The thread ID in which python is initialized
unsigned int g_hPythonThreadID;

#if defined (PY_CHECK_LOG_OVERRIDES) || defined (_DEBUG)
	ConVar py_log_overrides("py_log_overrides", "0", FCVAR_REPLICATED);
	ConVar py_log_overrides_module("py_log_overrides_module", "", FCVAR_REPLICATED);
#endif

// Global main space
bp::object mainmodule;
bp::object mainnamespace;

bp::object consolespace; // Used by spy/cpy commands

// Global module references.
bp::object builtins;
bp::object types;
bp::object sys;

bp::object weakref;
bp::object srcbuiltins;
bp::object srcbase;
bp::object steam;
bp::object _entitiesmisc;
bp::object _entities;
bp::object _particles;
bp::object _physics;
bp::object matchmaking;

#ifdef CLIENT_DLL
	boost::python::object _vguicontrols;
#endif // CLIENT_DLL

bp::object unit_helper;
bp::object srcmgr;
bp::object gamemgr;

boost::python::object fntype;

static CSrcPython g_SrcPythonSystem;

CSrcPython *SrcPySystem()
{
	return &g_SrcPythonSystem;
}

// Prevent python classes from initializing
bool g_bDoNotInitPythonClasses = true;

#ifdef CLIENT_DLL
extern void HookPyNetworkCls();
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Required by boost to be user defined if BOOST_NO_EXCEPTIONS is defined
//			http://www.boost.org/doc/libs/1_54_0/libs/exception/doc/throw_exception.html
//-----------------------------------------------------------------------------
namespace boost
{
	void throw_exception(std::exception const & e)
	{
		Warning("Boost Python Exception\n");

		FileHandle_t fh = filesystem->Open( "log.txt", "wb" );
		if ( !fh )
		{
			DevWarning( 2, "Couldn't create %s!\n", "log.txt" );
			return;
		}

		filesystem->Write( "Exception", strlen("Exception"), fh );
		filesystem->Close(fh);
	}
}

// Append functions
#ifdef CLIENT_DLL
extern void AppendClientModules();
#else
extern void AppendServerModules();
#endif // CLIENT_DLL
extern void AppendSharedModules();

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSrcPython::CSrcPython()
{
	m_bPythonRunning = false;
	m_bPythonIsFinalizing = false;
	m_bPathProtected = true;

	double fStartTime = Plat_FloatTime();
	// Before the python interpreter is initialized, the modules must be appended
#ifdef CLIENT_DLL
	AppendClientModules();
#else
	AppendServerModules();
#endif // CLIENT_DLL
	AppendSharedModules();

#ifdef CLIENT_DLL
	DevMsg( "CLIENT: " );
#else
	DevMsg( "SERVER: " );
#endif // CLIENT_DLL
	DevMsg( "Added Python default modules... (%f seconds)\n", Plat_FloatTime() - fStartTime );
}

bool CSrcPython::Init( )
{
	return InitInterpreter();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::Shutdown( void )
{
	if( !m_bPythonRunning )
		return;

#ifdef CLIENT_DLL
	PyShutdownProceduralMaterials();
#endif // CLIENT_DLL

	PyErr_Clear(); // Make sure it does not hold any references...
	GarbageCollect();
}

#ifdef WIN32
extern "C" {
	void PyImport_FreeDynLibraries( void );
}
#endif // WIN32

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::ExtraShutdown( void )
{
	ShutdownInterpreter();
}

static void VSPTCheckForParm( bp::list argv, const char *pParmToCheck )
{
	if( CommandLine()->FindParm( pParmToCheck ) == 0 )
		return;

	const char *value = CommandLine()->ParmValue( pParmToCheck, (const char *)0 );
	Msg("VSPT argument %s: %s\n", pParmToCheck, value ? value : "<null>" );
	argv.append( pParmToCheck );
	if( value )
		argv.append( value );
}

static void VSPTParmRemove( const char *pParmToCheck )
{
	if( CommandLine()->FindParm( pParmToCheck ) == 0 )
		return;

	CommandLine()->RemoveParm( pParmToCheck );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSrcPython::InitInterpreter( void )
{
	const bool bEnabled = CommandLine() && CommandLine()->FindParm("-disablepython") == 0;
	//const bool bToolsMode = CommandLine() && CommandLine()->FindParm("-tools") != 0;

	/*const char *pGameDir = COM_GetModDirectory();
	const char *pDevModDir = "hl2wars_asw_dev";
	if( V_strncmp( pGameDir, pDevModDir, V_strlen( pDevModDir ) ) != 0 )*/
	m_bPathProtected = CommandLine() ? CommandLine()->FindParm("-nopathprotection") == 0 : true;

	bool bNoChangeWorkingDirectory = CommandLine() ? CommandLine()->FindParm("-testnochangeworkingdir") != 0 : false;

	if( !bEnabled )
	{
	#ifdef CLIENT_DLL
		ConColorMsg( g_PythonColor, "CLIENT: " );
	#else
		ConColorMsg( g_PythonColor, "SERVER: " );
	#endif // CLIENT_DLL
		ConColorMsg( g_PythonColor, "Python is disabled.\n" );
		return true;
	}

	if( m_bPythonRunning )
		return true;

#ifdef CLIENT_DLL
	// WarsSplitscreen: only one player
	ACTIVE_SPLITSCREEN_PLAYER_GUARD( 0 );
#endif // CLIENT_DLL

#if 1
	if( !bNoChangeWorkingDirectory )
	{
		// Change working directory	
		char moddir[_MAX_PATH];
		filesystem->RelativePathToFullPath(".", "MOD", moddir, _MAX_PATH);
		V_FixupPathName(moddir, _MAX_PATH, moddir);	
		V_SetCurrentDirectory(moddir);
	}
#endif // 0

	m_bPythonRunning = true;

	double fStartTime = Plat_FloatTime();
	
#define PY_MAX_PATH 2048

	char buf[PY_MAX_PATH];
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
	filesystem->RelativePathToFullPath("python", "MOD", buf, sizeof(buf));
	V_FixupPathName(buf, sizeof(buf), buf);
	V_strcat( pythonhome, buf, sizeof(pythonhome) );
	
	// Set PYTHONPATH
	filesystem->RelativePathToFullPath("python/Lib", "MOD", buf, sizeof(buf));
	V_FixupPathName(buf, sizeof(buf), buf);
	V_strcat( pythonpath, buf, sizeof(pythonpath) );

#ifdef WIN32
	V_strcat( pythonpath, PYPATH_SEP, sizeof(pythonpath) );
#ifdef CLIENT_DLL
	filesystem->RelativePathToFullPath("python/Lib/ClientDLLs", "MOD", buf, sizeof(pythonpath));
	V_FixupPathName(buf, sizeof(pythonpath), buf);
	V_strcat( pythonpath, buf, sizeof(pythonpath) );
#else
	filesystem->RelativePathToFullPath("python/Lib/DLLs", "MOD", buf, sizeof(pythonpath));
	V_FixupPathName(buf, sizeof(pythonpath), buf);
	V_strcat( pythonpath, buf, sizeof(pythonpath) );
#endif // CLIENT_DLL
#endif // WIN32
	
#ifdef OSX
	V_strcat( pythonpath, PYPATH_SEP, sizeof(pythonpath) );
	filesystem->RelativePathToFullPath("python/Lib/plat-darwin", "MOD", buf, sizeof(pythonpath));
	V_FixupPathName(buf, sizeof(pythonpath), buf);
	V_strcat( pythonpath, buf, sizeof(pythonpath) );
	
	V_strcat( pythonpath, PYPATH_SEP, sizeof(pythonpath) );
	filesystem->RelativePathToFullPath("python", "MOD", buf, sizeof(pythonpath));
	V_FixupPathName(buf, sizeof(pythonpath), buf);
	V_strcat( pythonpath, buf, sizeof(pythonpath) );
#endif // OSX
	
#ifdef WIN32
	::SetEnvironmentVariable( "PYTHONHOME", pythonhome );
	::SetEnvironmentVariable( "PYTHONPATH", pythonpath );
#else
	::setenv( "PYTHONHOME", pythonhome, 1 );
    ::setenv( "PYTHONPATH", pythonpath, 1 );
#endif // WIN32

	//DevMsg( "PYTHONHOME: %s\nPYTHONPATH: %s\n", pythonhome, pythonpath );
    
#ifdef OSX
	Py_NoSiteFlag = 1;
#endif // OSX

	// Enable optimizations when not running in developer mode
	// This removes asserts and statements with "if __debug__"
#ifndef _DEBUG
	if( !developer.GetBool() )
		Py_OptimizeFlag = 1;
#endif // _DEBUG

	// Python 3 warnings
	const bool bPy3kWarnings = CommandLine() && CommandLine()->FindParm("-py3kwarnings") != 0;
	if( bPy3kWarnings )
	{
		Py_Py3kWarningFlag = 1;
		Py_BytesWarningFlag = 1;
		//Py_HashRandomizationFlag = 1;
	}

	// Initialize an interpreter
	Py_InitializeEx( 0 );
#ifdef CLIENT_DLL
	ConColorMsg( g_PythonColor, "CLIENT: " );
#else
	ConColorMsg( g_PythonColor, "SERVER: " );
#endif // CLIENT_DLL
	ConColorMsg( g_PythonColor, "Initialized Python... (%f seconds)\n", Plat_FloatTime() - fStartTime );
	fStartTime = Plat_FloatTime();

	// Save our thread ID
#ifdef _WIN32
	g_hPythonThreadID = GetCurrentThreadId();
#endif // _WIN32

	// get our main space
	try 
	{
		mainmodule = bp::import("__main__");
		mainnamespace = mainmodule.attr("__dict__");
	} 
	catch( bp::error_already_set & ) 
	{
		Warning("Failed to import main namespace!\n");
		PyErr_Print();
		return false;
	}

	// Import sys module
	Run( "import sys" );
	
	sys = Import("sys");

	// Redirect stdout/stderr to source print functions
	srcbuiltins = Import("srcbuiltins");
	sys.attr("stdout") = srcbuiltins.attr("SrcPyStdOut")();
	sys.attr("stderr") = srcbuiltins.attr("SrcPyStdErr")();

	weakref = Import("weakref");
	srcbase = Import("_srcbase");
	
#if PY_VERSION_HEX < 0x03000000
	builtins = Import("__builtin__");
#else
	builtins = Import("builtins");
#endif 

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
	catch( bp::error_already_set & ) 
	{
		PyErr_Print();
	}

	fntype = builtins.attr("type");

	// Add the maps directory to the modules path
	SysAppendPath("maps");
	SysAppendPath("python//srclib");

	// Default imports
	Import( "vmath" );

	srcmgr = Import("srcmgr");
	Run( "import srcmgr" );

	Import( "srcbase" );
	types = Import("types");
	steam = Import("steam");
	Run( "import sound" ); // Import _sound before _entitiesmisc (register converters)
	Run( "import _entitiesmisc" );
	_entitiesmisc = Import("_entitiesmisc");
	Run( "import _entities" );
	_entities = Import("_entities");
	unit_helper = Import("unit_helper");
	_particles = Import("_particles");
	_physics = Import("_physics");
	matchmaking = Import("matchmaking");
#ifdef CLIENT_DLL
	Run( "import input" );		// Registers buttons
	_vguicontrols = Import("_vguicontrols");
	Run( "import _cef" );
#endif	// CLIENT_DLL

	//  initialize the module that manages the python side
	Run( Get("_Init", "srcmgr", true) );
	
	// For spy/cpy commands
	consolespace = Import("consolespace");

#ifdef CLIENT_DLL
	DevMsg( "CLIENT: " );
#else
	DevMsg( "SERVER: " );
#endif // CLIENT_DLL
	DevMsg( "Initialized Python default modules... (%f seconds)\n", Plat_FloatTime() - fStartTime );

#ifdef WIN32
	if( !CheckVSPTInterpreter() )
		return false;

	if( !CheckVSPTDebugger() )
		return false;
#endif // WIN32

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSrcPython::ShutdownInterpreter( void )
{
	if( !m_bPythonRunning )
		return false;

	PyErr_Clear(); // Make sure it does not hold any references...
	GarbageCollect();

#ifdef CLIENT_DLL
	// Part of the main menu is in Python, so all windows must be closed when shutting down Python
	BaseModUI::CBaseModPanel::GetSingleton().CloseAllWindows();

	// Clear python panels
	DestroyPyPanels();
#endif // CLIENT_DLL

	// Clear Python gamerules
	if( PyGameRules().ptr() != Py_None )
		ClearPyGameRules();

	// Make sure these lists don't hold references
	m_deleteList.Purge();
	m_methodTickList.Purge();
	m_methodPerFrameList.Purge();

#ifdef CLIENT_DLL
	py_delayed_data_update_list.Purge();
#endif // CLIENT_DLL

	// Disconnect redirecting stdout/stderr
	sys.attr("stdout") = bp::object();
	sys.attr("stderr") = bp::object();

	// Clear modules
	mainmodule = bp::object();
	mainnamespace = bp::object();
	consolespace = bp::object();

	builtins = bp::object();
	srcbuiltins = bp::object();
	sys = bp::object();
	types = bp::object();
	srcmgr = bp::object();
	gamemgr = bp::object();
	weakref = bp::object();
	srcbase = bp::object();
	steam = bp::object();
	_entitiesmisc = bp::object();
	_entities = bp::object();
	unit_helper = bp::object();
	_particles = bp::object();
	_physics = bp::object();
	matchmaking = bp::object();
#ifdef CLIENT_DLL
	_vguicontrols = bp::object();
#endif // CLIENT_DLL

	fntype = bp::object();

	// Finalize
	m_bPythonIsFinalizing = true;
	PyErr_Clear(); // Make sure it does not hold any references...
	GarbageCollect();
	Py_Finalize();
#ifdef WIN32
	PyImport_FreeDynLibraries(); // IMPORTANT, otherwise it will crash if c extension modules are used.
#endif // WIN32
	m_bPythonIsFinalizing = false;
	m_bPythonRunning = false;

#ifdef CLIENT_DLL
	ConColorMsg( g_PythonColor, "CLIENT: " );
#else
	ConColorMsg( g_PythonColor, "SERVER: " );
#endif // CLIENT_DLL
	ConColorMsg( g_PythonColor, "Python is no longer running...\n" );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::PostInit()
{
	if( !IsPythonRunning() )
		return;

	// Hook PyMessage
#ifdef CLIENT_DLL
	// WarsSplitscreen: only one player
	ACTIVE_SPLITSCREEN_PLAYER_GUARD( 0 );

	HookPyMessage();
	HookPyNetworkCls();
	HookPyNetworkVar();
#endif // CLIENT_DLL

	// Gamemgr manages all game packages
	Run( "import gamemgr" );
	gamemgr = Import("gamemgr");

	// Autorun once
	ExecuteAllScriptsInPath("python/autorun_once/");
}

#ifdef WIN32
//-----------------------------------------------------------------------------
// Purpose: Support for Visual Studio Python Tools Interpreter
//-----------------------------------------------------------------------------
bool CSrcPython::CheckVSPTInterpreter()
{
#ifdef CLIENT_DLL
	return true; // TODO
#endif // CLIENT_DLL

	const bool bRunInterpreter = CommandLine() && CommandLine()->FindParm("-interpreter") != 0;
	if( !bRunInterpreter )
		return true;

#ifdef WIN32
	bool bSuccess = false;

#ifndef CLIENT_DLL
	tm today;
	Plat_GetLocalTime( &today );

	// Note: appends to the same log on the same day
	char logfilename[MAX_PATH];
	Q_snprintf( logfilename, sizeof( logfilename ), "con_logfile vspt_%02i-%02i-%04i.txt\n",
		today.tm_mon+1, today.tm_mday, 1900 + today.tm_year );

	engine->ServerCommand( logfilename );
	engine->ServerExecute();
#endif // CLIENT_DLL

	Msg("\n\n=======================================\n");
	Msg("Running in PySource Interpreter mode...\n");
	Msg("=======================================\n");

	// "-insecure" is added at the back. Remove it, so we can just take the remainder
	// after "-interpreter" argument.
	VSPTParmRemove("-insecure");

	std::string command("<null>");
	std::string cmdline = CommandLine()->GetCmdLine();
	std::string needle( "-interpreter " );
	std::size_t found = cmdline.find( needle );

	if( found == std::string::npos )
	{
		Warning("CheckVSPTInterpreter: -interpreter argument not found!\n");
		return true;
	}

	command = cmdline.substr( found + needle.length() );
	Msg( "Command: %s\n", command.c_str() );

	try
	{
		// Parse arguments for Python
		bp::str args( command.c_str() );

		bp::exec( "import shlex", mainnamespace, mainnamespace );
		bp::object shlex = Import("shlex");

		bp::str newCmd( args );
		newCmd = newCmd.replace( "\\\"", "\\\\\"" );

		builtins.attr("print")( newCmd );

		bp::list argv( shlex.attr("split")( newCmd ) );
		bp::setattr( sys, bp::object("argv"), argv );

		const char *pFilePath = bp::extract< const char * >( argv[0] );

		// Add path
		char vtptpath[MAX_PATH];
		V_strncpy( vtptpath, pFilePath, MAX_PATH );
		V_StripFilename( vtptpath );
		this->SysAppendPath( vtptpath );

		// Support 3 modes:
		// 1. Run a command with "-c"
		// 2. Run a module with "-m"
		// 3. Execute a file
		std::string firstchars = command.substr(0, 3);
		if( firstchars == "-c " )
		{
			command = command.substr( 3 );
			Msg("Running interpreter command: %s\n", command.c_str() );

			bp::exec( command.c_str() );
			bSuccess = true;
		}
		else if( firstchars == "-m " )
		{
			command = command.substr( 3 );
			Msg("Running interpreter module: %s\n", command.c_str() );

			bp::str newCmd( command.c_str() );
			newCmd = newCmd.replace( "\\\"", "\\\\\"" );

			builtins.attr("print")( newCmd );

			bp::list argv( shlex.attr("split")( newCmd ) );
			bp::setattr( sys, bp::object("argv"), argv );

			bp::object runpy = bp::import( "runpy" );
			boost::python::dict kwargs;
			kwargs["run_name"] = bp::object("__main__");
			kwargs["alter_sys"] = (bool)(true);
			runpy.attr("run_module")( *bp::make_tuple(argv[0]), **kwargs );
			bSuccess = true;
		}
		else
		{
			// Change working directory	
			V_SetCurrentDirectory( vtptpath );

			Msg("Running interpreter file: %s\n", pFilePath );
			Run("import encodings.idna");
			bSuccess = this->ExecuteFile( pFilePath );
		}
	}
	catch( bp::error_already_set & ) 
	{
		PyErr_Print();
	}

	if( !bSuccess )
	{
		Warning("PySource: Failed to execute interpreter file (%s). Please check log.\n", command.c_str() );
	}
	// Make sure logs are saved
#ifndef CLIENT_DLL
	engine->ServerExecute();
#endif // CLIENT_DLL
	filesystem->AsyncFinishAllWrites();
	::TerminateProcess( ::GetCurrentProcess(), 0 );
	return true;
#endif // WIN32
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSrcPython::CheckVSPTDebugger()
{
#ifdef CLIENT_DLL
	return true; // TODO
#endif // CLIENT_DLL

	const bool bRunDebugger = CommandLine() && CommandLine()->FindParm("-vsptdebug") != 0;
	if( !bRunDebugger )
		return true;

	Msg("Running VSPT Debug mode\n");

	// "-insecure" is added at the back. Remove it, so we can just take the remainder
	// after "-interpreter" argument.
	VSPTParmRemove("-insecure");

	std::string command("<null>");
	std::string cmdline = CommandLine()->GetCmdLine();
	std::string needle( "-vsptdebug " );
	std::size_t found = cmdline.find( needle );

	if( found != std::string::npos )
	{
		command = cmdline.substr( found + needle.length() );
		Msg( "Command: %s\n", command.c_str() );

		try
		{
			bp::object vsptdebug = bp::import("vsptdebug");

			vsptdebug.attr("run_vspt_debug")( command.c_str() );
		}
		catch( bp::error_already_set & ) 
		{
			PyErr_Print();
		}
	}

	return true;
}
#endif // WIN32

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::LevelInitPreEntity()
{
	m_bActive = true;

	if( !IsPythonRunning() )
		return;

#ifdef CLIENT_DLL
	char pLevelName[_MAX_PATH];
	V_FileBase(engine->GetLevelName(), pLevelName, _MAX_PATH);
#else
	const char *pLevelName = STRING(gpGlobals->mapname);
#endif

	m_LevelName = AllocPooledString(pLevelName);

	// BEFORE creating the entities setup the network tables
#ifndef CLIENT_DLL
	SetupNetworkTables();
#endif // CLIENT_DLL

	// srcmgr level init
	Run<const char *>( Get("_LevelInitPreEntity", "srcmgr", true), pLevelName );
	
	// Send prelevelinit signal
	try 
	{
		CallSignalNoArgs( Get("prelevelinit", "core.signals", true) );
		CallSignalNoArgs( Get("map_prelevelinit", "core.signals", true)[STRING(m_LevelName)] );
	} 
	catch( bp::error_already_set & ) 
	{
		Warning("Failed to retrieve level signal:\n");
		PyErr_Print();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::LevelInitPostEntity()
{
	if( !IsPythonRunning() )
		return;

	// srcmgr level init
	Run( Get("_LevelInitPostEntity", "srcmgr", true) );

	// Send postlevelinit signal
	try 
	{
		CallSignalNoArgs( Get("postlevelinit", "core.signals", true) );
		CallSignalNoArgs( Get("map_postlevelinit", "core.signals", true)[STRING(m_LevelName)] );
	} 
	catch( bp::error_already_set & ) 
	{
		Warning("Failed to retrieve level signal:\n");
		PyErr_Print();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::LevelShutdownPreEntity()
{
	if( !IsPythonRunning() )
		return;

	// srcmgr level shutdown
	Run( Get("_LevelShutdownPreEntity", "srcmgr", true) );

	// Send prelevelshutdown signal
	try 
	{
		CallSignalNoArgs( Get("prelevelshutdown", "core.signals", true) );
		CallSignalNoArgs( Get("map_prelevelshutdown", "core.signals", true)[STRING(m_LevelName)] );
	} 
	catch( bp::error_already_set & ) 
	{
		Warning("Failed to retrieve level signal:\n");
		PyErr_Print();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::LevelShutdownPostEntity()
{
	if( !IsPythonRunning() )
		return;

	// Clears references from the delete list, causing instances to cleanup if possible
	// Note: should not contain references to AutoGameSystems, because this is called from a gamesystem function
	//		 In this case crashes will occur.
	CleanupDeleteList();

	// srcmgr level shutdown
	Run( Get("_LevelShutdownPostEntity", "srcmgr", true) );

	// Send postlevelshutdown signal
	try 
	{
		CallSignalNoArgs( Get("postlevelshutdown", "core.signals", true) );
		CallSignalNoArgs( Get("map_postlevelshutdown", "core.signals", true)[STRING(m_LevelName)] );
	} 
	catch( bp::error_already_set & ) 
	{
		Warning("Failed to retrieve level signal:\n");
		PyErr_Print();
	}

	// Reset all send/recv tables
	PyResetAllNetworkTables();

	// Clear all tick signals next times (level time based)
	for( int i = m_methodTickList.Count() - 1; i >= 0; i--)
	{
		m_methodTickList[i].m_fNextTickTime = 0;
	}

	m_bActive = false;
}

static ConVar py_disable_update("py_disable_update", "0", FCVAR_CHEAT|FCVAR_REPLICATED);

#ifdef CLIENT_DLL
void CSrcPython::Update( float frametime )
#else
void CSrcPython::FrameUpdatePostEntityThink( void )
#endif // CLIENT_DLL
{
	if( !IsPythonRunning() )
		return;

	// Update tick methods
	int i;
	for( i = m_methodTickList.Count() - 1; i >= 0 ; i-- )
	{
		if( m_methodTickList[i].m_bUseRealTime )
			continue;

		if( m_methodTickList[i].m_fNextTickTime < gpGlobals->curtime )
		{
			try 
			{
				m_activeMethod = m_methodTickList[i].method;

				m_activeMethod();

				// Method might have removed the entry already
				if( !m_methodTickList.IsValidIndex(i) )
					continue;

				// Remove tick methods that are not looped (used to call back a function after a set time)
				if( !m_methodTickList[i].m_bLooped )
				{
					m_methodTickList.Remove( i );
					continue;
				}
			} 
			catch( bp::error_already_set & ) 
			{
				Warning("Unregistering tick method due the following exception (catch exception if you don't want this): \n");
				PyErr_Print();
				m_methodTickList.Remove( i );
				continue;
			}

			m_methodTickList[i].m_fNextTickTime = gpGlobals->curtime + m_methodTickList[i].m_fTickSignal;
		}	
	}

	// On the server this is called from CServerGameDLL::Think
#ifdef CLIENT_DLL
	UpdateRealtimeTickMethods();
#endif // CLIENT_DLL

	// Update frame methods
	for( i = m_methodPerFrameList.Count() - 1; i >= 0; i-- )
	{
		try 
		{
			m_activeMethod = m_methodPerFrameList[i];

			m_activeMethod();
		}
		catch( bp::error_already_set & ) 
		{
			Warning("Unregistering per frame method due the following exception (catch exception if you don't want this): \n");
			PyErr_Print();
			m_methodPerFrameList.Remove( i );
			continue;
		}
	}

	m_activeMethod = bp::object();

#ifdef CLIENT_DLL
	PyUpdateProceduralMaterials();

	CleanupDelayedUpdateList();
#endif // CLIENT_DLL

	// Clears references from the delete list, causing instances to cleanup if possible
	// Note: should not contain references to AutoGameSystems, because this is called from a gamesystem function
	//		 In this case crashes will occur.
	CleanupDeleteList();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::UpdateRealtimeTickMethods()
{
	if( !IsPythonRunning() )
		return;

	// Update tick methods
	int i;
	for( i = m_methodTickList.Count() - 1; i >= 0 ; i-- )
	{
		if( !m_methodTickList[i].m_bUseRealTime )
			continue;

		if( m_methodTickList[i].m_fNextTickTime < Plat_FloatTime() )
		{
			try 
			{
				m_activeMethod = m_methodTickList[i].method;

				m_activeMethod();

				// Method might have removed the entry already
				if( !m_methodTickList.IsValidIndex(i) )
					continue;

				// Remove tick methods that are not looped (used to call back a function after a set time)
				if( !m_methodTickList[i].m_bLooped )
				{
					m_methodTickList.Remove( i );
					continue;
				}
			} 
			catch( bp::error_already_set & ) 
			{
				Warning("Unregistering tick method due the following exception (catch exception if you don't want this): \n");
				PyErr_Print();
				m_methodTickList.Remove( i );
				continue;
			}

			m_methodTickList[i].m_fNextTickTime = Plat_FloatTime() + m_methodTickList[i].m_fTickSignal;
		}	
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bp::object CSrcPython::Import( const char *pModule )
{
	// Import into the main space
	try
	{
		return bp::import(pModule);
	}
	catch( bp::error_already_set & )
	{
#ifdef CLIENT_DLL
		DevMsg("CLIENT: ImportPyModuleIntern failed -> mod: %s\n", pModule );
#else
		DevMsg("SERVER: ImportPyModuleIntern failed -> mod: %s\n", pModule );
#endif

		PyErr_Print();
	}

	return bp::object();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bp::object CSrcPython::ImportSilent( const char *pModule )
{
	// Import into the main space
	try
	{
		return bp::import(pModule);
	}
	catch( bp::error_already_set & )
	{
		PyErr_Clear();
	}

	return bp::object();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bp::object CSrcPython::Get( const char *pAttrName, bp::object obj, bool bReport )
{
	try 
	{
		return obj.attr(pAttrName);
	}
	catch(bp::error_already_set &) 
	{
		if( bReport )
			PyErr_Print();	
		else
			PyErr_Clear();
	}
	return bp::object();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bp::object CSrcPython::Get( const char *pAttrName, const char *pModule, bool bReport )
{
	return Get( pAttrName, Import(pModule), bReport );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::Run( bp::object method, bool report_errors )
{
	PYRUNMETHOD( method, report_errors )
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::Run( const char *pString, const char *pModule )
{
	// execute statement
	try
	{
		bp::object dict = pModule ? Import(pModule).attr("__dict__") : mainnamespace;
		bp::exec( pString, dict, dict );
	}
	catch( bp::error_already_set & )
	{
		PyErr_Print();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSrcPython::ExecuteFile( const char* pScript )
{
	char char_filename[ _MAX_PATH ];
	char char_output_full_filename[ _MAX_PATH ];

	V_strncpy( char_filename, pScript, sizeof( char_filename ) );
	filesystem->RelativePathToFullPath( char_filename, "MOD", char_output_full_filename, sizeof( char_output_full_filename ) );

	const char *constcharpointer = reinterpret_cast<const char *>( char_output_full_filename );

	if( !filesystem->FileExists( constcharpointer ) )
	{
		Warning( "[Python] IFileSystem Cannot find the file: %s\n", constcharpointer );
		return false;
	}

	try
	{
		exec_file( constcharpointer, mainnamespace, mainnamespace );
	}
	catch( bp::error_already_set & )
	{
#ifdef CLIENT_DLL
		DevMsg( "CLIENT: " );
#else
		DevMsg( "SERVER: " );
#endif // CLIENT_DLL
		DevMsg( "RunPythonFile failed -> file: %s\n", pScript );
		PyErr_Print();
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::Reload( const char *pModule )
{
	DevMsg("Reloading module %s\n", pModule);

	try
	{
		// import into the main space
		char command[MAX_PATH];
		V_snprintf( command, sizeof( command ), "import %s", pModule );
		exec(command, mainnamespace, mainnamespace );

		// Reload
		V_snprintf( command, sizeof( command ), "reload(%s)", pModule );
		exec(command, mainnamespace, mainnamespace );
	}
	catch( bp::error_already_set & )
	{
		PyErr_Print();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Collect garbage
//-----------------------------------------------------------------------------
void CSrcPython::GarbageCollect( void )
{
	PyGC_Collect();
}

//-----------------------------------------------------------------------------
// Purpose: Add a path
//-----------------------------------------------------------------------------
void CSrcPython::SysAppendPath( const char* path, bool inclsubdirs )
{
	// First retrieve the append method
	bp::object append = Get("append", Get("path", "sys", true), true);

	// Fixup path
	char char_output_full_filename[ _MAX_PATH ];
	char p_out[_MAX_PATH];
	filesystem->RelativePathToFullPath(path,"GAME",char_output_full_filename,sizeof(char_output_full_filename));
	V_FixupPathName(char_output_full_filename, _MAX_PATH, char_output_full_filename );
	V_StrSubst(char_output_full_filename, "\\", "//", p_out, _MAX_PATH ); 

	// Append
	Run<const char *>( append, p_out, true );

	// Check for sub dirs
	if( inclsubdirs )
	{
		char wildcard[MAX_PATH];
		FileFindHandle_t findHandle;
		
		V_snprintf( wildcard, sizeof( wildcard ), "%s//*", path );
		const char *pFilename = filesystem->FindFirstEx( wildcard, "MOD", &findHandle );
		while ( pFilename != NULL )
		{

			if( V_strncmp(pFilename, ".", 1) != 0 &&
				V_strncmp(pFilename, "..", 2) != 0 &&
				filesystem->FindIsDirectory(findHandle) ) 
			{
				char path2[MAX_PATH];
				V_snprintf( path2, sizeof( path2 ), "%s//%s", path, pFilename );
				SysAppendPath(path2, inclsubdirs);
			}
			pFilename = filesystem->FindNext( findHandle );
		}
		filesystem->FindClose( findHandle );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Create a weakref using the weakref module
//-----------------------------------------------------------------------------
bp::object CSrcPython::CreateWeakRef( bp::object obj_ref )
{
	try
	{
		 return weakref.attr("ref")(obj_ref);
	}
	catch( bp::error_already_set & )
	{
		PyErr_Print();
	}
	return bp::object();
}

//-----------------------------------------------------------------------------
// Purpose: Execute all python files in a folder
//-----------------------------------------------------------------------------
void CSrcPython::ExecuteAllScriptsInPath( const char *pPath )
{
	char tempfile[MAX_PATH];
	char wildcard[MAX_PATH];

	V_snprintf( wildcard, sizeof( wildcard ), "%s*.py", pPath );

	FileFindHandle_t findHandle;
	const char *pFilename = filesystem->FindFirstEx( wildcard, "GAME", &findHandle );
	while ( pFilename != NULL )
	{
		V_snprintf( tempfile, sizeof( tempfile ), "%s/%s", pPath, pFilename );
		Msg("Executing %s\n", tempfile);
		ExecuteFile(tempfile);
		pFilename = filesystem->FindNext( findHandle );
	}
	filesystem->FindClose( findHandle );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::CallSignalNoArgs( bp::object signal )
{
	try 
	{
		srcmgr.attr("_CheckReponses")( 
			signal.attr("send_robust")( bp::object() )
		);
	} 
	catch( bp::error_already_set & ) 
	{
		Warning("Failed to call signal:\n");
		PyErr_Print();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::CallSignal( bp::object signal, bp::dict kwargs )
{
	try 
	{
		srcmgr.attr("_CallSignal")( 
			bp::object(signal.attr("send_robust")), kwargs 
		);
	} 
	catch( bp::error_already_set & ) 
	{
		Warning("Failed to call signal:\n");
		PyErr_Print();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Retrieving basic type values
//-----------------------------------------------------------------------------
int	CSrcPython::GetInt(const char *name, bp::object obj, int default_value, bool report_error )
{
	return Get<int>(name, obj, default_value, report_error);
}

float CSrcPython::GetFloat(const char *name, bp::object obj, float default_value, bool report_error )
{
	return Get<float>(name, obj, default_value, report_error);
}

const char *CSrcPython::GetString( const char *name, bp::object obj, const char *default_value, bool report_error )
{
	return Get<const char *>(name, obj, default_value, report_error);
}

Vector CSrcPython::GetVector( const char *name, bp::object obj, Vector default_value, bool report_error )
{
	return Get<Vector>(name, obj, default_value, report_error);
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
void CSrcPython::AddToDelayedUpdateList( EHANDLE hEnt, char *name, bp::object data, bool callchanged )
{
	CSrcPython::py_delayed_data_update v;
	v.hEnt = hEnt;
	V_strncpy( v.name, name, MAX_PATH );
	v.data = data;
	v.callchanged = callchanged;
	py_delayed_data_update_list.AddToTail( v );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::CleanupDelayedUpdateList()
{
	for( int i=py_delayed_data_update_list.Count()-1; i >= 0; i-- )
	{
		EHANDLE h = py_delayed_data_update_list[i].hEnt;
		if( h != NULL )
		{	
			if( g_debug_pynetworkvar.GetBool() )
			{
				DevMsg("#%d Cleaning up delayed PyNetworkVar update %s (callback: %d)\n", 
					h.GetEntryIndex(),
					py_delayed_data_update_list[i].name,
					py_delayed_data_update_list[i].callchanged );
			}
			
			h->PyUpdateNetworkVar( py_delayed_data_update_list[i].name, 
				py_delayed_data_update_list[i].data, py_delayed_data_update_list[i].callchanged );

			py_delayed_data_update_list.Remove(i);
		}
	}
}
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::RegisterTickMethod( bp::object method, float ticksignal, bool looped, bool userealtime )
{
	if( IsTickMethodRegistered( method ) )
	{
		PyErr_SetString(PyExc_Exception, "Method already registered" );
		throw boost::python::error_already_set(); 
	}

	py_tick_methods tickmethod;
	tickmethod.method = method;
	tickmethod.m_fTickSignal = ticksignal;
	tickmethod.m_fNextTickTime = (userealtime ? Plat_FloatTime() : gpGlobals->curtime) + ticksignal;
	tickmethod.m_bLooped = looped;
	tickmethod.m_bUseRealTime = userealtime;
	m_methodTickList.AddToTail(tickmethod);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::UnregisterTickMethod( bp::object method )
{
	if( m_activeMethod.ptr() != Py_None && method != m_activeMethod )
	{
		PyErr_SetString(PyExc_Exception, "Cannot remove methods from a tick method (other than the active tick method itself)" );
		throw boost::python::error_already_set(); 
	}

	for( int i = 0; i < m_methodTickList.Count(); i++ )
	{
		if( m_methodTickList[i].method == method )
		{
			m_methodTickList.Remove(i);
			return;
		}
	}

	PyErr_SetString(PyExc_Exception, VarArgs("Method %p not found", method.ptr()) );
	throw boost::python::error_already_set(); 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
boost::python::list CSrcPython::GetRegisteredTickMethods()
{
	boost::python::list methodlist;
	for( int i = 0; i < m_methodTickList.Count(); i++ )
		methodlist.append(m_methodTickList[i].method);
	return methodlist;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSrcPython::IsTickMethodRegistered( boost::python::object method )
{
#if defined( _DEBUG ) && defined( CLIENT_DLL )
	// FIXME!: This is broken in debug mode for some reason on the client, crashes on the bool operator
	// of bp::object. Disabled for now...
	return false;
#endif // _DEBUG

	for( int i = 0; i < m_methodTickList.Count(); i++ )
	{
		if( m_methodTickList[i].method == method )
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::RegisterPerFrameMethod( bp::object method )
{
	if( IsPerFrameMethodRegistered( method ) )
	{
		PyErr_SetString(PyExc_Exception, "Method already registered" );
		throw boost::python::error_already_set(); 
	}
	m_methodPerFrameList.AddToTail(method);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::UnregisterPerFrameMethod( bp::object method )
{
	if( m_activeMethod.ptr() != Py_None && method != m_activeMethod )
	{
		PyErr_SetString(PyExc_Exception, "Cannot remove methods from a perframe method (other than the active perframe method itself)" );
		throw boost::python::error_already_set(); 
	}

	for( int i = 0; i < m_methodPerFrameList.Count(); i++ )
	{
		if( m_methodPerFrameList[i] == method )
		{
			m_methodPerFrameList.Remove(i);
			return;
		}
	}
	PyErr_SetString(PyExc_Exception, VarArgs("Method %p not found", method.ptr()) );
	throw boost::python::error_already_set(); 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
boost::python::list CSrcPython::GetRegisteredPerFrameMethods()
{
	boost::python::list methodlist;
	for( int i = 0; i < m_methodPerFrameList.Count(); i++ )
		methodlist.append( m_methodPerFrameList[i] );
	return methodlist;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSrcPython::IsPerFrameMethodRegistered( boost::python::object method )
{
#if defined( _DEBUG ) && defined( CLIENT_DLL )
	// FIXME!: This is broken in debug mode for some reason on the client, crashes on the bool operator
	// of bp::object. Disabled for now...
	return false;
#endif // _DEBUG
	for( int i = 0; i < m_methodPerFrameList.Count(); i++ )
	{
		if( m_methodPerFrameList[i] == method )
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Commands follow here
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#ifndef CLIENT_DLL
CON_COMMAND( py_runfile, "Run a python script")
#else
CON_COMMAND_F( cl_py_runfile, "Run a python script", FCVAR_CHEAT)
#endif // CLIENT_DLL
{
	if( !SrcPySystem()->IsPythonRunning() )
		return;
#ifndef CLIENT_DLL
	if( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif // CLIENT_DLL
	g_SrcPythonSystem.ExecuteFile( args.ArgS() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#ifndef CLIENT_DLL
CON_COMMAND( spy, "Run a string on the python interpreter")
#else
CON_COMMAND_F( cpy, "Run a string on the python interpreter", FCVAR_CHEAT)
#endif // CLIENT_DLL
{
	if( !SrcPySystem()->IsPythonRunning() )
		return;
#ifndef CLIENT_DLL
	if( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif // CLIENT_DLL
	g_SrcPythonSystem.Run( args.ArgS(), "consolespace" );
}

#ifndef CLIENT_DLL
CON_COMMAND( py_restart, "Restarts the Python interpreter on the Server")
#else
CON_COMMAND_F( cl_py_restart, "Restarts the Python interpreter on the Client", FCVAR_CHEAT)
#endif // CLIENT_DLL
{
	if( !SrcPySystem()->IsPythonRunning() )
		return;
#ifndef CLIENT_DLL
	if( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif // CLIENT_DLL
	g_SrcPythonSystem.ShutdownInterpreter();
	g_SrcPythonSystem.InitInterpreter();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static int PyModuleAutocomplete( char const *partial, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] )
{
	int numMatches = 0;

	char newmodulepath[MAX_PATH];

	// Get command
	CUtlVector<char*, CUtlMemory<char*, int> > commandandrest;
	V_SplitString( partial, " ", commandandrest );
	if( commandandrest.Count() == 0 )
		return numMatches;
	char *pCommand = commandandrest[0];

	char *pRest = "";
	if( commandandrest.Count() > 1 )
		pRest = commandandrest[1];
	bool bEndsWithDot = pRest[V_strlen(pRest)-1] == '.';

	// Get modules typed so far
	CUtlVector<char*, CUtlMemory<char*, int> > modulesnames;
	V_SplitString( pRest, ".", modulesnames );

	// Construct path
	char path[MAX_PATH];
	path[0] = '\0';
	V_strcat( path, "python", MAX_PATH );
	char basemodulepath[MAX_PATH];
	basemodulepath[0] = '\0';

	// Add modules to path + remember base module path, stripping the last module
	for( int i = 0; i < modulesnames.Count(); i++ )
	{
		if( modulesnames.Count() - 1 == i && !bEndsWithDot )
			continue;

		V_strcat( path, "\\", MAX_PATH );
		V_strcat( path, modulesnames[i], MAX_PATH );

		V_strcat( basemodulepath, modulesnames[i], MAX_PATH );
		V_strcat( basemodulepath, ".", MAX_PATH );
	}

	char finalpath[MAX_PATH];
	V_FixupPathName( finalpath, MAX_PATH, path );
	
	// Create whildcar
	char wildcard[MAX_PATH];
	V_snprintf( wildcard, MAX_PATH, "%s\\*", finalpath );

	// List directories/filenames
	FileFindHandle_t findHandle;
	const char *filename = filesystem->FindFirstEx( wildcard, "MOD", &findHandle );
	while ( filename )
	{
		char fullpath[MAX_PATH];
		V_snprintf( fullpath, MAX_PATH, "%s/%s", finalpath, filename );

		if( V_strncmp( filename, ".", 1 ) == 0 || V_strncmp( filename, "..", 2 ) == 0 )
		{
			filename = filesystem->FindNext( findHandle );
			continue;
		}

		if( filesystem->IsDirectory( fullpath, "MOD" ) )
		{
			
			// Add directory if __init__.py inside
			char initpath[MAX_PATH];
			V_snprintf( initpath, MAX_PATH, "%s\\__init__.py", fullpath );
			if( filesystem->FileExists( initpath, "MOD" ) )
			{
				
				
				V_snprintf( newmodulepath, MAX_PATH, "%s%s", basemodulepath, filename );
				if( V_strncmp( pRest, newmodulepath, V_strlen(pRest) ) == 0 )
					V_snprintf( commands[ numMatches++ ], COMMAND_COMPLETION_ITEM_LENGTH, "%s %s", pCommand, newmodulepath );
			}
		}
		else
		{
			// Check extension. If .py, strip and add
			char ext[5];
			V_ExtractFileExtension( filename, ext, 5 );

			if( V_strncmp( ext, "py", 5 ) == 0 )
			{
				char noextfilename[MAX_PATH]; 
				V_StripExtension( filename, noextfilename, MAX_PATH );

				V_snprintf( newmodulepath, MAX_PATH, "%s%s", basemodulepath, noextfilename );
				if( V_strncmp( pRest, newmodulepath, V_strlen(pRest) ) == 0 )
					V_snprintf( commands[ numMatches++ ], COMMAND_COMPLETION_ITEM_LENGTH, "%s %s", pCommand, newmodulepath );
			}
		}

		if ( numMatches == COMMAND_COMPLETION_MAXITEMS )
			break;

		filename = filesystem->FindNext( findHandle );
	}
	filesystem->FindClose( findHandle );

	return numMatches;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#ifndef CLIENT_DLL
CON_COMMAND_F_COMPLETION( py_import, "Import a python module", 0, PyModuleAutocomplete )
#else
CON_COMMAND_F_COMPLETION( cl_py_import, "Import a python module", FCVAR_CHEAT, PyModuleAutocomplete )
#endif // CLIENT_DLL
{
	if( !SrcPySystem()->IsPythonRunning() )
		return;
#ifndef CLIENT_DLL
	if( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif // CLIENT_DLL
	char command[MAX_PATH];
	V_snprintf( command, sizeof( command ), "import %s", args.ArgS() );
	g_SrcPythonSystem.Run( command, "consolespace" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#ifndef CLIENT_DLL
CON_COMMAND_F_COMPLETION( py_reload, "Reload a python module", 0, PyModuleAutocomplete )
#else
CON_COMMAND_F_COMPLETION( cl_py_reload, "Reload a python module", FCVAR_CHEAT, PyModuleAutocomplete )
#endif // CLIENT_DLL
{
	if( !SrcPySystem()->IsPythonRunning() )
		return;
#ifndef CLIENT_DLL
	if( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif // CLIENT_DLL
	g_SrcPythonSystem.Reload( args.ArgS() );
}

#ifdef CLIENT_DLL
#include "vgui_controls/Panel.h"


CON_COMMAND_F( test_shared_converters, "Test server converters", FCVAR_CHEAT)
{
	if( !SrcPySystem()->IsPythonRunning() )
		return;
	Msg("Testing keyvalues converter\n");
	KeyValues *pFromPython;
	KeyValues *pToPython = new KeyValues("DataC", "CName1", "CValue1");

	pFromPython = SrcPySystem()->RunT<KeyValues *, KeyValues &>
		( SrcPySystem()->Get("test_keyvalues", "test_converters", true), NULL, *pToPython );

	if( pFromPython )
		Msg("Got keyvalues from python. Name: %s, Value1: %s\n", pFromPython->GetName(), pFromPython->GetString("Name1", ""));
	else
		Msg("No data from python :(\n");

	Msg("Testing string_t converter\n");
	string_t str_t_toPython = MAKE_STRING("Hello there");
	const char *str_from_python;
	str_from_python = SrcPySystem()->RunT<const char *, string_t>
		( SrcPySystem()->Get("test_string_t", "test_converters", true), NULL, str_t_toPython );
	Msg("Return value: %s\n", str_from_python);
}

CON_COMMAND_F( test_client_converters, "Test client converters", FCVAR_CHEAT)
{
	if( !SrcPySystem()->IsPythonRunning() )
		return;
	// Panel
	vgui::Panel *pFromPython;
	vgui::Panel *pToPython = new vgui::Panel(NULL, "PanelBla");

	pFromPython = SrcPySystem()->RunT<vgui::Panel *, vgui::Panel *>
		( SrcPySystem()->Get("test_panel", "test_converters", true), NULL, pToPython );

	if( pFromPython )
		Msg("Got Panel from python: %s\n", pFromPython->GetName() );
	else
		Msg("No data from python :(\n");
}

#endif // CLIENT_DLL
