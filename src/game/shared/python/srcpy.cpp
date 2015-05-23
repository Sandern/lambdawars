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
#include "tier1\fmtstr.h"

#ifdef CLIENT_DLL
#include "networkstringtable_clientdll.h"
#include "srcpy_materials.h"
#include "c_world.h"
#else
#include "networkstringtable_gamedll.h"
#include "world.h"
#endif // CLIENT_DLL

#ifdef WIN32
#include <winlite.h>
#endif // WIN32

//#include "../python/importlib.h" // Original
#include "srcpy_importlib.h" // Custom importlib for loading from vpk files

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

#if PY_VERSION_HEX >= 0x03000000
// Stubs for Python
const char *Py_GetBuildInfo(void) { return "SourcePy"; }
const char *_Py_hgversion(void) { return "1"; }
const char *_Py_hgidentifier(void) { return "srcpy"; }
#endif

#ifdef WIN32
extern "C" 
{
	char dllVersionBuffer[16] = ""; // a private buffer
	HMODULE PyWin_DLLhModule = NULL;
	const char *PyWin_DLLVersionString = dllVersionBuffer;

	// Needed to free dynamic imported modules (i.e. pyd modules)
	void PyImport_FreeDynLibraries( void );

	// Custom frozen modules
	static const struct _frozen _PyImport_FrozenModules[] = {
		/* importlib */
		{"_frozen_importlib", _Py_M__importlib, (int)sizeof(_Py_M__importlib)},
		{0, 0, 0} /* sentinel */
	};

}
#endif // WIN32

// For debugging
ConVar g_debug_python( "g_debug_python", "0", FCVAR_REPLICATED );

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
bp::object collections;
bp::object sys;

bp::object weakref;
bp::object srcbuiltins;
bp::object steam;
bp::object _entitiesmisc;
bp::object _entities;
bp::object _particles;
bp::object _physics;

#ifdef CLIENT_DLL
	boost::python::object _vguicontrols;
	bp::object _cef;
#endif // CLIENT_DLL

bp::object unit_helper;
bp::object srcmgr;
bp::object gamemgr;

boost::python::object fntype;
boost::python::object fnisinstance;

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

extern struct _inittab *PyImport_Inittab;
extern struct _inittab _PySourceImport_Inittab[];

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
	PyImport_Inittab = _PySourceImport_Inittab;

#ifdef CLIENT_DLL
	AppendClientModules();
#else
	AppendServerModules();
#endif // CLIENT_DLL
	AppendSharedModules();

	PyImport_FrozenModules = _PyImport_FrozenModules;

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

#define PY_MAX_PATH 2048

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSrcPython::InitInterpreter( void )
{
	const bool bEnabled = CommandLine() && CommandLine()->FindParm("-disablepython") == 0;
	//const bool bToolsMode = CommandLine() && CommandLine()->FindParm("-tools") != 0;

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

	filesystem->AddVPKFile( "python\\pythonlib_dir.vpk" );

	const int iVerbose = CommandLine() ? CommandLine()->ParmValue("-pythonverbose", 0) : 0;
	const int iDebugPython = CommandLine() ? CommandLine()->ParmValue("-pythondebug", 0) : 0;
	g_debug_python.SetValue( iDebugPython );

#ifdef CLIENT_DLL
	// WarsSplitscreen: only one player
	ACTIVE_SPLITSCREEN_PLAYER_GUARD( 0 );
#endif // CLIENT_DLL

	if( !bNoChangeWorkingDirectory )
	{
		// Change working directory	
		char moddir[MAX_PATH];
		filesystem->RelativePathToFullPath( ".", "MOD", moddir, sizeof( moddir ) );
		V_FixupPathName( moddir, sizeof( moddir ), moddir );	
		V_SetCurrentDirectory( moddir );
	}

	m_bPythonRunning = true;

	double fStartTime = Plat_FloatTime();

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
	V_strcat( pythonpath, "python/DLLs", sizeof(pythonpath) );
#endif // CLIENT_DLL
	V_strcat( pythonpath, PYPATH_SEP, sizeof(pythonpath) );
#endif // WIN32

	V_strcat( pythonpath, "python", sizeof(pythonpath) );
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

	V_strcat( pythonpath, "python/Lib", sizeof(pythonpath) );
	
#ifdef OSX
	V_strcat( pythonpath, "python/plat-darwin", sizeof(pythonpath) );
	V_strcat( pythonpath, "python", sizeof(pythonpath) );
#endif // OSX
	
	if( g_debug_python.GetBool() )
	{
		DevMsg("PYTHONPATH: %s (%d)\nPYTHONHOME: %s (%d)\n", 
			pythonpath, V_strlen(pythonpath), pythonhome, V_strlen(pythonhome));
	}

#ifdef WIN32
	::_putenv( VarArgs( "PYTHONHOME=%s", pythonhome ) );
	::_putenv( VarArgs( "PYTHONPATH=%s", pythonpath ) );
#else
	::setenv( "PYTHONHOME", pythonhome, 1 );
    ::setenv( "PYTHONPATH", pythonpath, 1 );
#endif // WIN32

	Py_NoSiteFlag = 1; // Not needed, so disabled

	// Enable optimizations when not running in developer mode
	// This removes asserts and statements with "if __debug__"
#ifndef _DEBUG
	if( !developer.GetBool() )
		Py_OptimizeFlag = 1;
#endif // _DEBUG

	if( iVerbose > 0 )
		Py_VerboseFlag = iVerbose;

	// Initialize an interpreter
	Py_InitializeEx( 0 );

	if( developer.GetInt() > 0 )
	{
#ifdef CLIENT_DLL
		ConColorMsg( g_PythonColor, "CLIENT: " );
#else
		ConColorMsg( g_PythonColor, "SERVER: " );
#endif // CLIENT_DLL
		ConColorMsg( g_PythonColor, "Initialized Python... (%f seconds)\n", Plat_FloatTime() - fStartTime );
	}
	fStartTime = Plat_FloatTime();

	// Shared initialization by standalone interpreter against game dll
	if( !PostInitInterpreter() )
	{
		return false;
	}

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
	if( !CheckVSPTDebugger() )
		return false;
#endif // WIN32

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSrcPython::PostInitInterpreter( bool bStandAloneInterpreter )
{
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
	sys = Import("sys");

	// Redirect stdout/stderr to source print functions
	// Also set these to the "original" values, which are none.
	srcbuiltins = Import("srcbuiltins");
	if( !bStandAloneInterpreter )
	{
		sys.attr("stdout") = srcbuiltins.attr("SrcPyStdOut")();
		sys.attr("stderr") = srcbuiltins.attr("SrcPyStdErr")();
		sys.attr("__stdout__") = sys.attr("stdout");
		sys.attr("__stderr__") = sys.attr("stderr");
	}

	weakref = Import("weakref");
	
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
	fnisinstance = builtins.attr("isinstance");

	// Default imports
	Import( "vmath" );

	// Most of these modules are not needed for the standalone interpreter, so 
	// skip them since they can have side effects.
	if( !bStandAloneInterpreter )
	{
		srcmgr = Import("srcmgr");

		types = Import("types");
		collections = Import("collections");
		steam = Import("steam");
		Run( "import sound" ); // Import _sound before _entitiesmisc (register converters)
		Run( "import _entitiesmisc" );
		_entitiesmisc = Import("_entitiesmisc");
		Run( "import _entities" );
		_entities = Import("_entities");
		unit_helper = Import("unit_helper");
		_particles = Import("_particles");
		_physics = Import("_physics");
#ifdef CLIENT_DLL
		Run( "import input" );		// Registers buttons
		_vguicontrols = Import("_vguicontrols");
		_cef = Import("_cef");
#endif	// CLIENT_DLL
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Shared shutdown code between standalone and game interpreter
//-----------------------------------------------------------------------------
bool CSrcPython::PreShutdownInterpreter( bool bIsStandAlone )
{
	PyErr_Clear(); // Make sure it does not hold any references...
	GarbageCollect();

	if( !bIsStandAlone )
	{
#ifdef CLIENT_DLL
		// Clear python panels
		DestroyPyPanels();
#endif // CLIENT_DLL

		// Clear Python gamerules
		if( PyGameRules().ptr() != Py_None )
		{
			// Ingame: install default c++ gamerules
#ifdef CLIENT_DLL
			if( GetClientWorldEntity() )
#else
			if( GetWorldEntity() )
#endif // CLIENT_DLL
			{
				PyInstallGameRules( boost::python::object() );
			}
			else
			{
				ClearPyGameRules();
			}
		}

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
	}

	// Clear modules
	mainmodule = bp::object();
	mainnamespace = bp::object();
	consolespace = bp::object();

	builtins = bp::object();
	srcbuiltins = bp::object();
	sys = bp::object();
	types = bp::object();
	collections = bp::object();
	srcmgr = bp::object();
	gamemgr = bp::object();
	weakref = bp::object();
	steam = bp::object();
	_entitiesmisc = bp::object();
	_entities = bp::object();
	unit_helper = bp::object();
	_particles = bp::object();
	_physics = bp::object();
#ifdef CLIENT_DLL
	_vguicontrols = bp::object();
	_cef = bp::object();
#endif // CLIENT_DLL

	fntype = bp::object();
	fnisinstance = bp::object();

	// Finalize
	m_bPythonIsFinalizing = true;
	PyErr_Clear(); // Make sure it does not hold any references...
	GarbageCollect();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Shared shutdown code between standalone and game interpreter
//-----------------------------------------------------------------------------
bool CSrcPython::PostShutdownInterpreter( bool bIsStandAlone )
{
#ifdef WIN32
	PyImport_FreeDynLibraries(); // IMPORTANT, otherwise it will crash if c extension modules are used.
#endif // WIN32

	m_bPythonIsFinalizing = false;
	m_bPythonRunning = false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSrcPython::ShutdownInterpreter( void )
{
	if( !m_bPythonRunning )
		return false;

	if( !PreShutdownInterpreter() )
		return false;

	Py_Finalize();

	if( !PostShutdownInterpreter() )
		return false;

	if( developer.GetInt() > 0 )
	{
#ifdef CLIENT_DLL
		ConColorMsg( g_PythonColor, "CLIENT: " );
#else
		ConColorMsg( g_PythonColor, "SERVER: " );
#endif // CLIENT_DLL
		ConColorMsg( g_PythonColor, "Python is no longer running...\n" );
	}

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

	PostInitInterpreterActions();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::PostInitInterpreterActions( void )
{
	// Gamemgr manages all game packages
	Run( "import gamemgr" );
	gamemgr = Import("gamemgr");

	// Autorun once
	ExecuteAllScriptsInPath("python/autorun_once/");
}

#ifdef WIN32
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

	V_strncpy( m_LevelName, pLevelName, sizeof( m_LevelName ) );

	// BEFORE creating the entities setup the network tables
#ifndef CLIENT_DLL
	SetupNetworkTables();
#endif // CLIENT_DLL

	// srcmgr level init
	Run<const char *>( Get("_LevelInitPreEntity", "srcmgr", true), m_LevelName );
	
	// Send prelevelinit signal
	try 
	{
		CallSignalNoArgs( Get("prelevelinit", "core.signals", true) );
		CallSignalNoArgs( Get("map_prelevelinit", "core.signals", true)[m_LevelName] );
	} 
	catch( bp::error_already_set & ) 
	{
		Warning( "Failed to retrieve pre level init signal (level name: %s):\n", m_LevelName );
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
		CallSignalNoArgs( Get("map_postlevelinit", "core.signals", true)[m_LevelName] );
	} 
	catch( bp::error_already_set & ) 
	{
		Warning( "Failed to retrieve post level init signal (level name: %s):\n", m_LevelName );
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
		CallSignalNoArgs( Get("map_prelevelshutdown", "core.signals", true)[m_LevelName] );
	} 
	catch( bp::error_already_set & ) 
	{
		Warning( "Failed to retrieve pre level shutdown signal (level name: %s):\n", m_LevelName );
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
		CallSignalNoArgs( Get("map_postlevelshutdown", "core.signals", true)[m_LevelName] );
	} 
	catch( bp::error_already_set & ) 
	{
		Warning( "Failed to retrieve post level shutdown signal (level name: %s):\n", m_LevelName );
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
				if( !m_methodTickList.IsValidIndex(i) || m_methodTickList[i].method != m_activeMethod )
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
				if( !m_methodTickList.IsValidIndex(i) || m_methodTickList[i].method != m_activeMethod )
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

	m_activeMethod = boost::python::object();
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
	Run( pString, pModule ? Import(pModule).attr("__dict__") : mainnamespace );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::Run( const char *pString, boost::python::object module )
{
	// execute statement
	try
	{
		bp::object dict = module.ptr() != Py_None ? module.attr("__dict__") : mainnamespace;
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
	if( !filesystem->FileExists( pScript ) )
	{
		Warning( "[Python] IFileSystem Cannot find the file: %s\n", pScript );
		return false;
	}

	try
	{
		// Read in file using source filesystem, to prevent issues with encoding in path name
		PyObject *filename;
		filename = PyUnicode_DecodeFSDefault(pScript);
		if (filename == NULL)
		{
			Warning( "[Python] IFileSystem Cannot find the file: %s\n", pScript );
			return false;
		}

		boost::python::object global = mainnamespace;
		boost::python::object local = mainnamespace;

		// Set suitable default values for global and local dicts.
		if (global.is_none())
		{
			if (PyObject *g = PyEval_GetGlobals())
				global = boost::python::object(boost::python::detail::borrowed_reference(g));
			else
				global = boost::python::dict();
		}
		if (local.is_none()) local = global;

		CUtlBuffer content;
		if( !filesystem->ReadFile( pScript, NULL, content ) )
		{
			Warning( "[Python] IFileSystem Cannot find the file: %s\n", pScript );
			Py_DECREF(filename);
			return false;
		}

		PyCompilerFlags cf;
		cf.cf_flags = PyCF_SOURCE_IS_UTF8;

		PyObject *v = PyRun_StringFlags((const char *)content.Base(), Py_file_input, global.ptr(), local.ptr(), &cf);
		if( v )
		{
			Py_DECREF(v);
		}

		Py_DECREF(filename);
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
	//char char_output_full_filename[MAX_PATH];
	//char p_out[MAX_PATH];
	//filesystem->RelativePathToFullPath( path, NULL, char_output_full_filename, sizeof( char_output_full_filename ) );
	//V_FixupPathName( char_output_full_filename, sizeof( char_output_full_filename ), char_output_full_filename );
	//V_StrSubst( char_output_full_filename, "\\", "/", p_out, sizeof( p_out ) ); 
	
	// Append
	Run<const char *>( append, path, true );

	// Check for sub dirs
	if( inclsubdirs )
	{
		char wildcard[MAX_PATH];
		FileFindHandle_t findHandle;
		
		V_snprintf( wildcard, sizeof( wildcard ), "%s/*", path );
		const char *pFilename = filesystem->FindFirstEx( wildcard, NULL, &findHandle );
		while ( pFilename != NULL )
		{

			if( V_strncmp(pFilename, ".", 1) != 0 &&
				V_strncmp(pFilename, "..", 2) != 0 &&
				filesystem->FindIsDirectory(findHandle) ) 
			{
				char path2[MAX_PATH];
				V_snprintf( path2, sizeof( path2 ), "%s/%s", path, pFilename );
				SysAppendPath( path2, inclsubdirs );
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
	const char *pFilename = filesystem->FindFirstEx( wildcard, NULL, &findHandle );
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
void CSrcPython::PreProcessDelayedUpdates( CBaseEntity *pEntity )
{
	for( int i = py_delayed_data_update_list.Count() - 1; i >= 0; i-- )
	{
		EHANDLE h = py_delayed_data_update_list[i].hEnt;
		if( h == pEntity )
		{	
			if( g_debug_pynetworkvar.GetBool() )
			{
				DevMsg("#%d Processing delayed PyNetworkVar update %s (callback: %d)\n", 
					h.GetEntryIndex(),
					py_delayed_data_update_list[i].name,
					py_delayed_data_update_list[i].callchanged );
			}
			
			h->PyUpdateNetworkVar( py_delayed_data_update_list[i].name, 
				py_delayed_data_update_list[i].data, false, true );

			if( !py_delayed_data_update_list[i].callchanged )
				py_delayed_data_update_list.Remove(i);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::PostProcessDelayedUpdates( CBaseEntity *pEntity )
{
	for( int i = py_delayed_data_update_list.Count() - 1; i >= 0; i-- )
	{
		EHANDLE h = py_delayed_data_update_list[i].hEnt;
		if( h == pEntity )
		{	
			if( g_debug_pynetworkvar.GetBool() )
			{
				DevMsg("#%d Post Processing delayed PyNetworkVar update %s (callback: %d)\n", 
					h.GetEntryIndex(),
					py_delayed_data_update_list[i].name,
					py_delayed_data_update_list[i].callchanged );
			}
			
			// Dispatch changed callback if needed
			if( py_delayed_data_update_list[i].callchanged )
			{
				h->PyNetworkVarChanged( py_delayed_data_update_list[i].name );
			}

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

	m_methodTickList.AddToTail();
	py_tick_methods &tickmethod = m_methodTickList.Tail();
	tickmethod.method = method;
	tickmethod.m_fTickSignal = ticksignal;
	tickmethod.m_fNextTickTime = (userealtime ? Plat_FloatTime() : gpGlobals->curtime) + ticksignal;
	tickmethod.m_bLooped = looped;
	tickmethod.m_bUseRealTime = userealtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSrcPython::UnregisterTickMethod( bp::object method )
{
	if( m_activeMethod.ptr() != Py_None && method != m_activeMethod )
	{
		PyErr_SetString(PyExc_Exception, "Cannot remove methods from tick method (other than the active tick method itself)" );
		throw boost::python::error_already_set(); 
	}

	for( int i = 0; i < m_methodTickList.Count(); i++ )
	{
		// HACK for debug mode, but unstable. bool_type operator causes a crash.
#if defined( CLIENT_DLL ) && defined( _DEBUG )
		if( PyObject_IsTrue( (m_methodTickList[i].method == method).ptr() ) )
#else
		if( m_methodTickList[i].method == method )
#endif
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
	for( int i = 0; i < m_methodTickList.Count(); i++ )
	{
		// HACK for debug mode, but unstable. bool_type operator causes a crash.
#if defined( CLIENT_DLL ) && defined( _DEBUG )
		if( PyObject_IsTrue( (m_methodTickList[i].method == method).ptr() ) )
			return true;
#else
		if( m_methodTickList[i].method == method )
			return true;
#endif 
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
		PyErr_SetString(PyExc_Exception, "Cannot remove methods from perframe method (other than the active perframe method itself)" );
		throw boost::python::error_already_set(); 
	}

	for( int i = 0; i < m_methodPerFrameList.Count(); i++ )
	{
		// HACK for debug mode, but unstable. bool_type operator causes a crash.
#if defined( CLIENT_DLL ) && defined( _DEBUG )
		if( PyObject_IsTrue( (m_methodPerFrameList[i] == method).ptr() ) )
#else
		if( m_methodPerFrameList[i] == method )
#endif
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
	for( int i = 0; i < m_methodPerFrameList.Count(); i++ )
	{
		// HACK for debug mode, but unstable. bool_type operator causes a crash.
#if defined( CLIENT_DLL ) && defined( _DEBUG )
		if( PyObject_IsTrue( (m_methodPerFrameList[i].method == method).ptr() ) )
			return true;
#else
		if( m_methodPerFrameList[i] == method )
			return true;
#endif 
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
	g_SrcPythonSystem.Run( args.ArgS(), consolespace );
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
	g_SrcPythonSystem.PostInitInterpreterActions();
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

			if( V_strncmp( ext, "py", 2 ) == 0 )
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
	g_SrcPythonSystem.Run( command, consolespace );
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
