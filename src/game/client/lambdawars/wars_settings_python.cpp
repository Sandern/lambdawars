//=============================================================================//
//
// Purpose: Hand-written Python module "_gamesettings" for the settings page of
//			the menu. Gives access to what the native option dialogs
//			(gameui/wars/vvideo.cpp and vaudio.cpp) use and Python can't reach:
//			the video modes, the anti-aliasing modes of the graphics card, the
//			recommended video config, applying a video mode, the voice tweak API
//			and the audio language. ConVars are read and set from Python.
//			Registered in CSrcPython::CSrcPython (srcpy.cpp).
//
//=============================================================================//

#include "cbase.h"
#include "srcpy.h"
#include "IGameUIFuncs.h"
#include "modes.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/materialsystem_config.h"
#include "videocfg/videocfg.h"
#include "videocfgext.h"
#include "ivoicetweak.h"
#include "steam/steam_api.h"
#include "vgui/IVGui.h"
#include "gameui/wars/basemodpanel.h"
#include "gameui/wars/vaudio.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

namespace bp = boost::python;

int GetScreenAspectMode( int width, int height );

//-----------------------------------------------------------------------------
// Video
//-----------------------------------------------------------------------------

// Fullscreen modes as (width, height), sorted by width, then height
static bp::list PyGetVideoModes()
{
	bp::list modes;
	vmode_t *plist = NULL;
	int count = 0;
	gameuifuncs->GetVideoModes( &plist, &count );
	for( int i = 0; i < count; i++ )
		modes.append( bp::make_tuple( plist[i].width, plist[i].height ) );
	return modes;
}

static bp::tuple PyGetDesktopResolution()
{
	int width, height;
	gameuifuncs->GetDesktopResolution( width, height );
	return bp::make_tuple( width, height );
}

// 0 for normal (4:3), 1 for 16:9 and 2 for 16:10, like the aspect ratio choice of the video dialog
static int PyGetScreenAspectMode( int width, int height )
{
	return GetScreenAspectMode( width, height );
}

static bp::dict PyGetVideoMode()
{
	const MaterialSystem_Config_t &config = materials->GetCurrentConfigForVideoCard();
	bp::dict mode;
	mode["width"] = config.m_VideoMode.m_Width;
	mode["height"] = config.m_VideoMode.m_Height;
	mode["windowed"] = config.Windowed();
	mode["noborder"] = config.NoWindowBorder();
	return mode;
}

// The anti-aliasing modes the graphics card supports as (samples, quality), in the order of
// the video dialog. The first is no anti-aliasing.
static bp::list PyGetAntialiasModes()
{
	bp::list modes;
	modes.append( bp::make_tuple( 1, 0 ) );
	if( materials->SupportsMSAAMode( 2 ) )
		modes.append( bp::make_tuple( 2, 0 ) );
	if( materials->SupportsMSAAMode( 4 ) )
		modes.append( bp::make_tuple( 4, 0 ) );
	if( materials->SupportsMSAAMode( 6 ) )
		modes.append( bp::make_tuple( 6, 0 ) );
	if( materials->SupportsCSAAMode( 4, 2 ) )
		modes.append( bp::make_tuple( 4, 2 ) );
	if( materials->SupportsCSAAMode( 4, 4 ) )
		modes.append( bp::make_tuple( 4, 4 ) );
	if( materials->SupportsMSAAMode( 8 ) )
		modes.append( bp::make_tuple( 8, 0 ) );
	if( materials->SupportsCSAAMode( 8, 2 ) )
		modes.append( bp::make_tuple( 8, 2 ) );
	return modes;
}

// The recommended settings of the video config ("Use recommended settings"), or None
static bp::object PyGetRecommendedVideoSettings()
{
	KeyValues *pConfigKeys = new KeyValues( "VideoConfig" );
	if( !ReadCurrentVideoConfig( pConfigKeys, true ) )
	{
		pConfigKeys->deleteThis();
		return bp::object();
	}

	bp::dict settings;
	settings["width"] = pConfigKeys->GetInt( "setting.defaultres", 640 );
	settings["height"] = pConfigKeys->GetInt( "setting.defaultresheight", 480 );
	settings["windowed"] = !pConfigKeys->GetBool( "setting.fullscreen", true );
	settings["noborder"] = pConfigKeys->GetBool( "setting.nowindowborder", false );
	settings["gpu_mem_level"] = clamp( pConfigKeys->GetInt( "setting.gpu_mem_level", 0 ), 0, 2 );
	settings["mem_level"] = clamp( pConfigKeys->GetInt( "setting.mem_level", 0 ), 0, 2 );
	settings["mat_antialias"] = pConfigKeys->GetInt( "setting.mat_antialias", 0 );
	settings["mat_aaquality"] = pConfigKeys->GetInt( "setting.mat_aaquality", 0 );
	settings["mat_forceaniso"] = pConfigKeys->GetInt( "setting.mat_forceaniso", 1 );
	settings["mat_vsync"] = pConfigKeys->GetBool( "setting.mat_vsync", true );
	settings["mat_triplebuffered"] = pConfigKeys->GetBool( "setting.mat_triplebuffered", false );
	settings["gpu_level"] = pConfigKeys->GetInt( "setting.gpu_level", 0 );
	settings["cpu_level"] = pConfigKeys->GetInt( "setting.cpu_level", 0 );
	settings["mat_grain_scale_override"] = pConfigKeys->GetFloat( "setting.mat_grain_scale_override", 1.0f );
	settings["mat_queue_mode"] = pConfigKeys->GetInt( "setting.mat_queue_mode", -1 );
	settings["in_lock_mouse_to_window"] = pConfigKeys->GetBool( "setting.in_lock_mouse_to_window", true );
	pConfigKeys->deleteThis();
	return settings;
}

// Applies the video mode and saves the video settings, after Python set the ConVars of the
// video dialog. Same as Video::ApplyChanges.
static void PyApplyVideoSettings( int width, int height, bool bWindowed, bool bNoBorder )
{
	const MaterialSystem_Config_t &config = materials->GetCurrentConfigForVideoCard();
	if( config.m_VideoMode.m_Width != width || config.m_VideoMode.m_Height != height ||
		config.Windowed() != bWindowed || config.NoWindowBorder() != bNoBorder )
	{
		engine->ClientCmd_Unrestricted( VarArgs( "mat_setvideomode %i %i %i %i\n", width, height, bWindowed ? 1 : 0, bNoBorder ? 1 : 0 ) );
	}

	engine->ClientCmd_Unrestricted( "mat_savechanges\n" );
	engine->ClientCmd_Unrestricted( VarArgs( "host_writeconfig_ss %d", XBX_GetActiveUserId() ) );

	// The video dialog passes the current mode, the new mode is only set when the command runs
	UpdateCurrentVideoConfig( width, height, GetScreenAspectMode( width, height ), !bWindowed, bNoBorder );
	SaveVideoCfgExt();
}

//-----------------------------------------------------------------------------
// Voice, through the voice tweak API like the audio dialog. Not available
// without a microphone.
//-----------------------------------------------------------------------------
static IVoiceTweak *GetVoiceTweak()
{
	return engine->GetVoiceTweakAPI();
}

// None without voice tweak API, else the microphone and voice volumes (0-1) and microphone boost
static bp::object PyGetVoiceSettings()
{
	IVoiceTweak *pVoiceTweak = GetVoiceTweak();
	if( !pVoiceTweak )
		return bp::object();

	bp::dict settings;
	settings["micvolumefound"] = pVoiceTweak->IsControlFound( MicrophoneVolume );
	settings["micvolume"] = pVoiceTweak->GetControlFloat( MicrophoneVolume );
	settings["receivevolume"] = pVoiceTweak->GetControlFloat( OtherSpeakerScale );
	settings["micboost"] = pVoiceTweak->GetControlFloat( MicBoost ) != 0.0f;
	return settings;
}

static void PySetVoiceSettings( float micVolume, float receiveVolume, bool bMicBoost )
{
	IVoiceTweak *pVoiceTweak = GetVoiceTweak();
	if( !pVoiceTweak )
		return;
	pVoiceTweak->SetControlFloat( MicrophoneVolume, micVolume );
	pVoiceTweak->SetControlFloat( OtherSpeakerScale, receiveVolume );
	pVoiceTweak->SetControlFloat( MicBoost, bMicBoost ? 1.0f : 0.0f );
}

// Echoes the microphone back, see GetMicrophoneLevel. Returns false if the test can't start.
static bool PyStartMicrophoneTest( float micVolume )
{
	IVoiceTweak *pVoiceTweak = GetVoiceTweak();
	if( !pVoiceTweak )
		return false;
	pVoiceTweak->SetControlFloat( MicrophoneVolume, micVolume );
	return pVoiceTweak->StartVoiceTweakMode() != 0;
}

static void PyStopMicrophoneTest()
{
	IVoiceTweak *pVoiceTweak = GetVoiceTweak();
	if( !pVoiceTweak || !pVoiceTweak->IsStillTweaking() )
		return;
	pVoiceTweak->EndVoiceTweakMode();

	// Restarts the open microphone, like the audio dialog
	ConVarRef voice_vox( "voice_vox" );
	if( voice_vox.GetBool() )
	{
		voice_vox.SetValue( 0 );
		voice_vox.SetValue( 1 );
	}
}

// Level of the microphone during the test (0-1), or -1 when not testing
static float PyGetMicrophoneLevel()
{
	IVoiceTweak *pVoiceTweak = GetVoiceTweak();
	if( !pVoiceTweak || !pVoiceTweak->IsStillTweaking() )
		return -1.0f;
	return pVoiceTweak->GetControlFloat( SpeakingVolume );
}

//-----------------------------------------------------------------------------
// Audio language, only shown when the game has more than one
//-----------------------------------------------------------------------------
static bp::tuple PyGetAudioLanguages()
{
	bp::list languages;
	if( !steamapicontext || !steamapicontext->SteamApps() )
		return bp::make_tuple( "", languages );

	CSplitString languagesList( steamapicontext->SteamApps()->GetAvailableGameLanguages(), "," );
	for( int i = 0; i < languagesList.Count(); i++ )
		languages.append( (const char *)languagesList[i] );
	return bp::make_tuple( steamapicontext->SteamApps()->GetCurrentGameLanguage(), languages );
}

// Quits and restarts the game through Steam in the language, like the audio dialog
static void PyRestartWithAudioLanguage( const char *pszLanguage )
{
	// Kept until the restart, the audio dialog only keeps a pointer
	static char s_szLanguage[64];
	V_strncpy( s_szLanguage, pszLanguage, sizeof( s_szLanguage ) );
	BaseModUI::Audio::SetUpdatedAudioLanguage( s_szLanguage );
	vgui::ivgui()->PostMessage( BaseModUI::CBaseModPanel::GetSingleton().GetVPanel(),
		new KeyValues( "command", "command", "RestartWithNewLanguage" ), 0, 0.01f );
}

BOOST_PYTHON_MODULE(_gamesettings)
{
	bp::def( "GetVideoModes", PyGetVideoModes );
	bp::def( "GetDesktopResolution", PyGetDesktopResolution );
	bp::def( "GetScreenAspectMode", PyGetScreenAspectMode );
	bp::def( "GetVideoMode", PyGetVideoMode );
	bp::def( "GetAntialiasModes", PyGetAntialiasModes );
	bp::def( "GetRecommendedVideoSettings", PyGetRecommendedVideoSettings );
	bp::def( "ApplyVideoSettings", PyApplyVideoSettings );

	bp::def( "GetVoiceSettings", PyGetVoiceSettings );
	bp::def( "SetVoiceSettings", PySetVoiceSettings );
	bp::def( "StartMicrophoneTest", PyStartMicrophoneTest );
	bp::def( "StopMicrophoneTest", PyStopMicrophoneTest );
	bp::def( "GetMicrophoneLevel", PyGetMicrophoneLevel );

	bp::def( "GetAudioLanguages", PyGetAudioLanguages );
	bp::def( "RestartWithAudioLanguage", PyRestartWithAudioLanguage );
}
