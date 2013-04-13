//====== Copyright © 2013 Sandern Corporation, All rights reserved. ===========//
//
// Purpose: Wars specific input handling
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "hl2wars_in_main.h"
#include "kbutton.h"
#include "in_buttons.h"

#include "c_hl2wars_player.h"
#include "wars_mapboundary.h"
#include "vgui/isurface.h"
#include "vgui_controls/controls.h"
#include "iclientmode.h"
#include "vgui_controls/Panel.h"
#include "ienginevgui.h"
#include "ivieweffects.h"

#include "engine/IVDebugOverlay.h"
#include "vgui/hl2warsviewport.h"

#include <vgui/IInput.h>

#include "prediction.h"
#include "checksum_md5.h"
#include "hltvcamera.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern int g_iAlive;
extern ConVar sv_noclipduringpause;

extern ConVar in_forceuser;
extern int in_impulse[ MAX_SPLITSCREEN_PLAYERS ];

void CameraSetttingsChanged( IConVar *var, const char *pOldValue, float flOldValue )
{
	C_HL2WarsPlayer *pPlayer = C_HL2WarsPlayer::GetLocalHL2WarsPlayer();
	if( !pPlayer )
		return;

	engine->ClientCmd("player_camerasettings");
}


// ConVars
ConVar cl_mouse_edgespeed( "cl_mouse_edgespeed", "450.0", FCVAR_CLIENTDLL|FCVAR_ARCHIVE, "The speed of movement when we bring the mouse to the edges of the screen." );
ConVar cl_mouse_edgethreshold( "cl_mouse_edgethreshold", "5", FCVAR_CLIENTDLL|FCVAR_ARCHIVE, "Threshold around edge of the screen at which the mouse will start moving the player (in normalized pixels)" );
ConVar cl_mouse_clamptoscreen( "cl_mouse_clamptoscreen", "1", FCVAR_CLIENTDLL|FCVAR_ARCHIVE, "Clamp mouse to within screen." );

ConVar cl_strategic_cam_rot_speed( "cl_strategic_cam_rot_speed", "10.0", FCVAR_CLIENTDLL|FCVAR_ARCHIVE, "Rotation speed (degrees per second)");
ConVar cl_strategic_cam_rot_maxdelta( "cl_strategic_cam_rot_maxdelta", "10.0", FCVAR_CLIENTDLL|FCVAR_ARCHIVE, "Clamps max mouse delta by which the rotation speed is scaled");
ConVar cl_strategic_cam_pitchdown( "cl_strategic_cam_pitchdown", "90", FCVAR_ARCHIVE );
ConVar cl_strategic_cam_pitchup( "cl_strategic_cam_pitchup", "-40", FCVAR_ARCHIVE );

ConVar cl_strategic_cam_speed( "cl_strategic_cam_speed", "2000.0", FCVAR_ARCHIVE|FCVAR_USERINFO, "", CameraSetttingsChanged );
ConVar cl_strategic_cam_accelerate( "cl_strategic_cam_accelerate", "6.0", FCVAR_ARCHIVE|FCVAR_USERINFO, "", CameraSetttingsChanged );
ConVar cl_strategic_cam_stopspeed( "cl_strategic_cam_stopspeed", "200.0", FCVAR_ARCHIVE|FCVAR_USERINFO, "", CameraSetttingsChanged );
ConVar cl_strategic_cam_friction( "cl_strategic_cam_friction", "6.0", FCVAR_ARCHIVE|FCVAR_USERINFO, "", CameraSetttingsChanged );

ConVar cl_strategic_cam_scrolltimeout( "cl_strategic_cam_scrolltimeout", "0.1", FCVAR_ARCHIVE, "The amount of time scrolling keeps going on." );
ConVar cl_strategic_cam_scrolldelta( "cl_strategic_cam_scrolldelta", "250", FCVAR_ARCHIVE );

ConVar cl_strategic_height_tol( "cl_strategic_cam_height_tol", "48.0", FCVAR_ARCHIVE, "Height tolerance" );
ConVar cl_strategic_height_adjustspeed( "cl_strategic_height_adjustspeed", "4000.0", FCVAR_ARCHIVE, "Speed at which the camera adjusts to the terrain height." );

ConVar cl_debug_movement_hint( "cl_debug_movement_hint", "0" );

extern void OnActiveConfigChanged( IConVar *var, const char *pOldValue, float flOldValue );
ConVar cl_active_config( "cl_active_config", "config_fps", FCVAR_ARCHIVE, "Active binding configuration file. Auto changes.", OnActiveConfigChanged );

extern ConVar cl_pitchdown;
extern ConVar cl_pitchup;
extern ConVar cl_sidespeed;
extern ConVar cl_upspeed;
extern ConVar cl_forwardspeed;
extern ConVar cl_backspeed;

extern kbutton_t	in_strafe;
extern kbutton_t	in_up;
extern kbutton_t	in_down;
extern kbutton_t	in_left;
extern kbutton_t	in_right;
extern kbutton_t	in_lookspin; // left rotation
extern kbutton_t	in_zoom; // right rotation

// Camera settings
#define CAM_MIN_DIST 30.0
#define	DIST	 2

extern IClientMode *GetClientModeNormal();

static CHL2WarsInput g_Input;

// Expose this interface
::IInput *input = ( ::IInput * )&g_Input;

CHL2WarsInput::CHL2WarsInput()
{
	m_bStrategicMode = false;
	m_bResetDHNextSample = false;
}

void CHL2WarsInput::Init_All( void )
{
	CInput::Init_All();

	BuildWindowList();
}

void CHL2WarsInput::Shutdown_All( void )
{
	CInput::Shutdown_All();

	char config[MAX_PATH];
	Q_snprintf( config, MAX_PATH, "%s.cfg", cl_active_config.GetString());

	SwitchConfig( config, NULL, NULL );
}

void CHL2WarsInput::LevelInit( void )
{
	CInput::LevelInit();

	m_flCurrentCameraDist = -1;
	m_flDesiredCameraDist = -1;
	m_fLastOriginZ = -1;
	m_fScrollTimeOut = 0.0f;
	m_bScrolling = false;

	// Get minimum camera distance
	ConVarRef cl_strategic_cam_min_dist("cl_strategic_cam_min_dist");

	char buf[MAX_PATH];
	V_StripExtension( engine->GetLevelName(), buf, MAX_PATH );
	Q_snprintf( buf, sizeof( buf ), "%s.res", buf );

	KeyValues *pMapData = new KeyValues("MapData");
	if( pMapData->LoadFromFile( filesystem, buf ) )
	{
		cl_strategic_cam_min_dist.SetValue( pMapData->GetString("cam_mindist", cl_strategic_cam_min_dist.GetDefault()) );

		pMapData->deleteThis();
		pMapData = NULL;
	}
	else
	{
		cl_strategic_cam_min_dist.SetValue( cl_strategic_cam_min_dist.GetDefault() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#define LINE_BUF_SIZE 1024
void CHL2WarsInput::GetBindings(const char *config, CUtlBuffer &bufout)
{
	CUtlBuffer buf(0, 0, CUtlBuffer::TEXT_BUFFER);
	filesystem->ReadFile(VarArgs("cfg/%s", config), "MOD", buf);

	char curline[LINE_BUF_SIZE];
	int i;
	do {
		buf.GetLine(curline, LINE_BUF_SIZE);
		if( !Q_strncmp(curline, "unbindall", 9) || !Q_strncmp(curline, "bind", 4) )
		{
			for(i=0; i<LINE_BUF_SIZE; i++) 
			{
				if( curline[i] == 0 )
					break;
				bufout.PutChar(curline[i]);
			}
		}
		else
		{
			break;
		}
	} while( buf.IsValid() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2WarsInput::SwitchConfig(const char *pSaveCurConfigTo, 
								   const char *pSwitchToConfig, const char *pSwitchToConfigDefault)
{
	if( pSaveCurConfigTo )
	{
		filesystem->AsyncFinishAllWrites(); // Ensure we have the right config, because host_writeconfig uses async write
		CUtlBuffer buf;
		GetBindings("config.cfg", buf);
		filesystem->WriteFile(VarArgs("cfg/%s", pSaveCurConfigTo), "MOD", buf);
		filesystem->AsyncFinishAllWrites(); // Make sure the current config is written away
	}

	if( pSwitchToConfig )
	{
		if( !filesystem->FileExists(VarArgs("cfg/%s", pSwitchToConfig)) )
		{
			if( !pSwitchToConfigDefault || !filesystem->FileExists(VarArgs("cfg/%s", pSwitchToConfigDefault)) )
			{
				Warning("SwitchConfig: No default config %s for config %s!\n", pSwitchToConfig, pSwitchToConfigDefault);
				return;	
			}

			CUtlBuffer buf;
			filesystem->ReadFile(VarArgs("cfg/%s", pSwitchToConfigDefault), "MOD", buf);
			filesystem->WriteFile(VarArgs("cfg/%s", pSwitchToConfig), "MOD", buf);
		}
		engine->ExecuteClientCmd( VarArgs("exec %s\n", pSwitchToConfig) );
		engine->ExecuteClientCmd( "host_writeconfig\n" ); // Writes it into config.cfg
		
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void OnActiveConfigChanged( IConVar *var, const char *pOldValue, float flOldValue )
{
	char curconfig[MAX_PATH];
	Q_snprintf( curconfig, MAX_PATH, "%s.cfg", pOldValue);
	char newconfig[MAX_PATH];
	Q_snprintf( newconfig, MAX_PATH, "%s.cfg", cl_active_config.GetString());
	char newconfigdefault[MAX_PATH];
	Q_snprintf( newconfigdefault, MAX_PATH, "%s_default.cfg", cl_active_config.GetString());

	if( Q_stricmp( curconfig, newconfig ) == 0 )
		return;

	static bool bInitialConfig = true;
	Msg("Config changed from %s to %s\n", bInitialConfig ? "none" : curconfig, newconfig);
	g_Input.SwitchConfig( bInitialConfig ? NULL : curconfig, newconfig, newconfigdefault );
	bInitialConfig = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2WarsInput::CreateMove ( int sequence_number, float input_sample_frametime, bool active )
{
	m_fCurrentSampleTime = input_sample_frametime;

#if 0
	// We won't do this in Init_all, since it will do a "exec config.cfg"
	static bool bConfigInitialized = false;
	if( !bConfigInitialized )
	{
		// Start with fps bindings
		m_bStrategicMode = false;
		SwitchConfig(NULL, "config_fps.cfg", "config_fps_default.cfg");
		bConfigInitialized = true;
		return;
	}
#endif // 0

	C_HL2WarsPlayer *pPlayer = C_HL2WarsPlayer::GetLocalHL2WarsPlayer();
	if( !pPlayer )
	{
		CInput::CreateMove( sequence_number, input_sample_frametime, active);
		return;
	}

	if( pPlayer->IsStrategicModeOn() != m_bStrategicMode )
	{
		//Msg("Strategic mode changed %d -> %d\n", m_bStrategicMode, pPlayer->IsStrategicModeOn());
		m_bStrategicMode = pPlayer->IsStrategicModeOn();
		if( m_bStrategicMode )
		{
			cl_active_config.SetValue("config_rts");
		}
		else
		{
			cl_active_config.SetValue("config_fps");
		}
	}

	// Use baseclass for none rts mode.
	if( !pPlayer->IsStrategicModeOn() )
	{
		CInput::CreateMove( sequence_number, input_sample_frametime, active);
		return;
	}

	ASSERT_LOCAL_PLAYER_RESOLVABLE();
	int nSlot = GET_ACTIVE_SPLITSCREEN_SLOT();

	CUserCmd *cmd = &GetPerUser( nSlot ).m_pCommands[ sequence_number % MULTIPLAYER_BACKUP];
	CVerifiedUserCmd *pVerified = &GetPerUser( nSlot ).m_pVerifiedCommands[ sequence_number % MULTIPLAYER_BACKUP];

	cmd->Reset();

	cmd->command_number = sequence_number;
	cmd->tick_count = gpGlobals->tickcount;

	QAngle viewangles;

	if ( active || sv_noclipduringpause.GetInt() )
	{
		if ( nSlot == in_forceuser.GetInt() )
		{

			// Determine view angles
			AdjustAngles ( nSlot, input_sample_frametime );

			// Determine movement
			ComputeMoveSpeed( input_sample_frametime );

			CapAndSetSpeed( cmd );
		}

		// Allow mice and other controllers to add their inputs
		ControllerMove( nSlot, input_sample_frametime, cmd );
	}
	else
	{
		// need to run and reset mouse input so that there is no view pop when unpausing
		if ( !GetPerUser( nSlot ).m_fCameraInterceptingMouse && m_fMouseActive )
		{
			float mx, my;
			GetAccumulatedMouseDeltasAndResetAccumulators( nSlot, &mx, &my );
			ResetMouse();
		}
	}
	// Retreive view angles from engine ( could have been set in IN_AdjustAngles above )
	engine->GetViewAngles( viewangles );

	cmd->impulse = in_impulse[ nSlot ];
	in_impulse[ nSlot ] = 0;

	// Latch and clear weapon selection
	if ( GetPerUser( nSlot ).m_hSelectedWeapon != NULL )
	{
		C_BaseCombatWeapon *weapon = GetPerUser( nSlot ).m_hSelectedWeapon;

		cmd->weaponselect = weapon->entindex();
		cmd->weaponsubtype = weapon->GetSubType();

		// Always clear weapon selection
		GetPerUser( nSlot ).m_hSelectedWeapon = NULL;
	}

	// Set button and flag bits
	cmd->buttons = GetButtonBits( true );

	// Using joystick?
	if ( in_joystick.GetInt() )
	{
		if ( cmd->forwardmove > 0 )
		{
			cmd->buttons |= IN_FORWARD;
		}
		else if ( cmd->forwardmove < 0 )
		{
			cmd->buttons |= IN_BACK;
		}
	}

	// Use new view angles if alive, otherwise user last angles we stored off.
	VectorCopy( viewangles, cmd->viewangles );
	VectorCopy( viewangles, GetPerUser( nSlot ).m_angPreviousViewAngles );

	// Let the move manager override anything it wants to.
	if ( GetClientMode()->CreateMove( input_sample_frametime, cmd ) )
	{
		// Get current view angles after the client mode tweaks with it
		engine->SetViewAngles( cmd->viewangles );
	}

	CheckPaused( cmd );

	CUserCmd *pPlayer0Command = &m_PerUser[ 0 ].m_pCommands[ sequence_number % MULTIPLAYER_BACKUP ];
	CheckSplitScreenMimic( nSlot, cmd, pPlayer0Command );

	GetPerUser( nSlot ).m_flLastForwardMove = cmd->forwardmove;

	cmd->random_seed = MD5_PseudoRandom( sequence_number ) & 0x7fffffff;

	HLTVCamera()->CreateMove( cmd );
#if defined( REPLAY_ENABLED )
	ReplayCamera()->CreateMove( cmd );
#endif

#if defined( HL2_CLIENT_DLL )
	// copy backchannel data
	int i;
	for (i = 0; i < GetPerUser( nSlot ).m_EntityGroundContact.Count(); i++)
	{
		cmd->entitygroundcontact.AddToTail( GetPerUser().m_EntityGroundContact[i] );
	}
	GetPerUser( nSlot ).m_EntityGroundContact.RemoveAll();
#endif

	pVerified->m_cmd = *cmd;
	pVerified->m_crc = cmd->GetChecksum();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2WarsInput::ExtraMouseSample( float frametime, bool active )
{
	m_fCurrentSampleTime = frametime;

	C_HL2WarsPlayer *pPlayer = C_HL2WarsPlayer::GetLocalHL2WarsPlayer();
	if( !pPlayer )
	{
		CInput::ExtraMouseSample( frametime, active );
		return;
	}

	// Use baseclass for none rts mode.
	if( !pPlayer->IsStrategicModeOn() )
	{
		CInput::ExtraMouseSample( frametime, active );
		return;
	}

	ASSERT_LOCAL_PLAYER_RESOLVABLE();
	int nSlot = GET_ACTIVE_SPLITSCREEN_SLOT();

	static CUserCmd dummy[ MAX_SPLITSCREEN_PLAYERS ];
	CUserCmd *cmd = &dummy[ nSlot ];

	cmd->Reset();

	QAngle viewangles;

	if ( active )
	{

		// Determine view angles
		AdjustAngles ( nSlot, frametime );

		// Determine movement
		ComputeMoveSpeed( frametime );

		CapAndSetSpeed( cmd );

		// Allow mice and other controllers to add their inputs
		ControllerMove( nSlot, frametime, cmd );
	}

	// Retrieve view angles from engine ( could have been set in AdjustAngles above )
	engine->GetViewAngles( viewangles );

	// Set button and flag bits, don't blow away state
	cmd->buttons = GetButtonBits( false );

	// Use new view angles if alive, otherwise user last angles we stored off.
	VectorCopy( viewangles, cmd->viewangles );
	VectorCopy( viewangles, GetPerUser().m_angPreviousViewAngles );

	// Let the move manager override anything it wants to.
	if ( GetClientMode()->CreateMove( frametime, cmd ) )
	{
		// Get current view angles after the client mode tweaks with it
		engine->SetViewAngles( cmd->viewangles );
		prediction->SetLocalViewAngles( cmd->viewangles );
	}

	CheckPaused( cmd );

	CheckSplitScreenMimic( nSlot, cmd, &dummy[ 0 ] );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2WarsInput::SetScrollTimeOut(bool forward)
{
	if( forward )
		m_flDesiredCameraDist += cl_strategic_cam_scrolldelta.GetFloat();
	else
		m_flDesiredCameraDist -= cl_strategic_cam_scrolldelta.GetFloat();

	m_bScrollForward = forward;
	m_fScrollTimeOut = gpGlobals->curtime + cl_strategic_cam_scrolltimeout.GetFloat();
	m_bScrolling = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CHL2WarsInput::ComputeScrollSpeed( float frametime )
{
	float fScrollSpeed = 0.0f;

	return fScrollSpeed;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2WarsInput::ComputeMoveSpeed( float frametime )
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2WarsInput::CapAndSetSpeed( CUserCmd *cmd )
{
	int nSlot = GET_ACTIVE_SPLITSCREEN_SLOT();

	// X and Y axis movement
	ComputeForwardMove( nSlot, cmd );
	ComputeSideMove( nSlot, cmd );

	// Determine zoom (only used if the map has no boundary)
	ComputeUpwardMove( nSlot, cmd );

	// Determine rotation
	QAngle	viewangles;	
	engine->GetViewAngles( viewangles );

	float fRotSpeed = (-100 * (in_lookspin.GetPerUser().state & 1)) + (100 * (in_zoom.GetPerUser().state & 1));

	viewangles[YAW] = anglemod(viewangles[YAW] + (fRotSpeed * m_fCurrentSampleTime));
	engine->SetViewAngles( viewangles );

	if( !GetMapBoundaryList() )
	{
		if( m_fScrollTimeOut < gpGlobals->curtime )
		{
			m_bScrolling = false;
		}
		else
		{
			if( m_bScrollForward )
				cmd->upmove = -450.0f;
			else
				cmd->upmove = 450.0f;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CHL2WarsInput::GetButtonBits( bool bResetState )
{
	int bits = CInput::GetButtonBits(bResetState);

	HL2WarsViewport *pViewPort = dynamic_cast<HL2WarsViewport *>(GetClientModeNormal()->GetViewport());
	if( !pViewPort )
		return bits;

	// Add mouse buttons
	bits |= pViewPort->m_nMouseButtons;

	return bits;
}


//-----------------------------------------------------------------------------
// Purpose: Do not reset the mouse in strategic mode
//			We might want to remove the calls to ResetMouse, but I'm not sure
//			where it is called from in some cases.
//-----------------------------------------------------------------------------
void CHL2WarsInput::ResetMouse( void )
{
	// Do not reset dah mouse in strategic mode
	C_HL2WarsPlayer *pPlayer = C_HL2WarsPlayer::GetLocalHL2WarsPlayer();
	if( pPlayer && (pPlayer->IsStrategicModeOn() || pPlayer->ForceShowMouse() /*(pPlayer->m_nButtons & IN_SPEED)*/ ) )
		return;

	CInput::ResetMouse();
}

//-----------------------------------------------------------------------------
// Purpose: In strategic mode we want the real mouse position from GetFullscreenMousePos
//			Not the delta from the center
//-----------------------------------------------------------------------------
void CHL2WarsInput::GetFullscreenMousePos( int *mx, int *my, int *unclampedx, int *unclampedy )
{
	C_HL2WarsPlayer *pPlayer = C_HL2WarsPlayer::GetLocalHL2WarsPlayer();
	if( !pPlayer || pPlayer->IsStrategicModeOn() == false) {
		CInput::GetFullscreenMousePos(mx, my, unclampedx, unclampedy);
		return;
	}

	Assert( mx );
	Assert( my );

	int x, y;
	GetWindowCenter( x,  y );

	int		current_posx, current_posy;

	GetMousePos(current_posx, current_posy);

	current_posx -= x;
	current_posy -= y;

	// Now need to add back in mid point of viewport
	//

	int w, h;
	vgui::surface()->GetScreenSize( w, h );
	current_posx += w  / 2;
	current_posy += h / 2;

	if ( unclampedx )
	{
		*unclampedx = current_posx;
	}

	if ( unclampedy )
	{
		*unclampedy = current_posy;
	}

	// Clamp
	current_posx = MAX( -1, current_posx );
	current_posx = MIN( ScreenWidth()+1, current_posx );

	current_posy = MAX( -1, current_posy );
	current_posy = MIN( ScreenHeight()+1, current_posy );

	*mx = current_posx;
	*my = current_posy;
}

//-----------------------------------------------------------------------------
// Purpose: Update the aim of the mouse pointer
//-----------------------------------------------------------------------------
void CHL2WarsInput::UpdateMouseAim( C_HL2WarsPlayer *pPlayer, int x, int y )
{
	Vector forward;
	// Remap x and y into -1 to 1 normalized space 
	float xf, yf; 
	xf = ( 2.0f * x / (float)(ScreenWidth()-1) ) - 1.0f; 
	yf = ( 2.0f * y / (float)(ScreenHeight()-1) ) - 1.0f; 

	// Flip y axis 
	yf = -yf; 

	const VMatrix &worldToScreen = engine->WorldToScreenMatrix(); 
	VMatrix screenToWorld; 
	MatrixInverseGeneral( worldToScreen, screenToWorld ); 

	// Create two points at the normalized mouse x, y pos and at the near and far z planes (0 and 1 depth) 
	Vector v1, v2; 
	v1.Init( xf, yf, 0.0f ); 
	v2.Init( xf, yf, 1.0f ); 

	Vector o2; 
	// Transform the two points by the screen to world matrix 
	screenToWorld.V3Mul( v1, pPlayer->Weapon_ShootPosition() + pPlayer->GetCameraOffset() ); // ray start origin 
	screenToWorld.V3Mul( v2, o2 ); // ray end origin 
	VectorSubtract( o2, pPlayer->Weapon_ShootPosition() + pPlayer->GetCameraOffset(), forward ); 
	forward.NormalizeInPlace(); 
	pPlayer->UpdateMouseData( forward );
}

void CHL2WarsInput::ControllerMove ( int nSlot, float frametime, CUserCmd *cmd )
{
	// If the gameui is open, disable mouse input and do not update mouse aim
	// This way it will keep being at the last position (in case you type something in the console)
	if( enginevgui->IsGameUIVisible() )
	{
		DeactivateMouseClipping();

		// Disable mouse if necessary
		vgui::Panel *pViewPort = GetClientMode()->GetViewport();
		if( pViewPort->IsMouseInputEnabled() )
			pViewPort->SetMouseInputEnabled(false);

		CInput::ControllerMove( nSlot, frametime, cmd );
		return;
	}

	CHL2WarsInput::MouseMove( nSlot, cmd );
}

//-----------------------------------------------------------------------------
// Purpose: Update mouse aim and add movement if mouse goes along edges
//-----------------------------------------------------------------------------
void CHL2WarsInput::MouseMove ( int nSlot, CUserCmd *cmd )
{
	// Mouse rotation active?
	m_bMouseRotationActive = (in_walk.GetPerUser().state & 1);
	
	C_HL2WarsPlayer *pPlayer = C_HL2WarsPlayer::GetLocalHL2WarsPlayer();
	if( !pPlayer )
		return;

	// Get mouse coordinates
	int x, y, nx, ny, nw, nh;
	GetMousePos(x, y);

	// Save delta
	m_iDeltaX = x - m_iLastPosX;
	m_iDeltaY = y - m_iLastPosY;

	if( cl_mouse_clamptoscreen.GetInt() && !enginevgui->IsGameUIVisible() && HasWindowFocus() )
	{
		ActivateMouseClipping();
	}
	else
	{
		DeactivateMouseClipping();

		// Don't update if the mouse is outside the screenspace
		if( x < 0 || x > ScreenWidth() || 
			y < 0 || y > ScreenHeight() )
			return;
	}

	// Normalize
	nx = vgui::scheme()->GetProportionalNormalizedValue(x);
	ny = vgui::scheme()->GetProportionalNormalizedValue(y);
	nw = vgui::scheme()->GetProportionalNormalizedValue(ScreenWidth());
	nh = vgui::scheme()->GetProportionalNormalizedValue(ScreenHeight());

	// Update mouse aim (done in both fps & rts mode)
	UpdateMouseAim( pPlayer, x, y);
	//cmd->m_vMouseAim = pPlayer->GetMouseAim();

	// Call baseclass if not in strategic mouse
	if( pPlayer->IsStrategicModeOn() == false ) {
		// Disable mouse if necessary
		vgui::Panel *pViewPort = GetClientMode()->GetViewport();
		if( !pPlayer->ForceShowMouse()/*!(pPlayer->m_nButtons & IN_SPEED)*/ )
		{
			if( pViewPort->IsMouseInputEnabled() )
				pViewPort->SetMouseInputEnabled(false);
		}
		else
		{
			if( pViewPort->IsMouseInputEnabled() == false )
				pViewPort->SetMouseInputEnabled(true);
		}
		CInput::MouseMove(nSlot,cmd);
		return;
	}
	
	// Enable/disable mouse if not already
	HL2WarsViewport *pViewPort = dynamic_cast<HL2WarsViewport *>( GetClientMode()->GetViewport() );
	if( !pViewPort )
		return;

	if( m_bMouseRotationActive )
	{
		if( pViewPort->IsMouseInputEnabled() )
			pViewPort->SetMouseInputEnabled(false);
	}
	else if( !pViewPort->IsMouseInputEnabled() )
	{
		pViewPort->SetMouseInputEnabled(true);
	}

	if( !pViewPort->IsSelecting() )
	{
		if( pViewPort->IsMiddleMouseButtonPressed() )
		{
			cmd ->forwardmove -= cl_mouse_edgespeed.GetFloat() * m_iDeltaY;
			cmd ->sidemove += cl_mouse_edgespeed.GetFloat() * m_iDeltaX;

			// Reset mouse to last mouse position
			SetMousePos( m_iLastPosX, m_iLastPosY );
		}
		else if( m_bMouseRotationActive )
		{
			// Rotation?
			UpdateMouseRotation();

			// Reset mouse to last mouse position
			SetMousePos( m_iLastPosX, m_iLastPosY );
		}
		else
		{
			// Add to movement if our mouse goes along the edges
			if( x < cl_mouse_edgethreshold.GetInt() ) {
				cmd ->sidemove -= cl_mouse_edgespeed.GetFloat() *
				 MAX( MIN( 1.0-(nx/(cl_mouse_edgethreshold.GetInt()/2.0) ),1.0 ),0.0 );
			} else if( x > nw - cl_mouse_edgethreshold.GetInt() ) {
				cmd ->sidemove += cl_mouse_edgespeed.GetFloat() *
				MAX( MIN( 1.0-((nw-nx-1)/(cl_mouse_edgethreshold.GetInt()/2.0) ),1.0 ),0.0 );
			}

			if( y < cl_mouse_edgethreshold.GetInt() ) {
				cmd ->forwardmove += cl_mouse_edgespeed.GetFloat() * 
				MAX( MIN( 1.0-(ny/(cl_mouse_edgethreshold.GetInt()/2.0) ),1.0 ),0.0 );
			} else if( y > nh - cl_mouse_edgethreshold.GetInt() ) {
				cmd ->forwardmove -= cl_mouse_edgespeed.GetFloat() * 
				MAX( MIN( 1.0-((nh-ny-1)/(cl_mouse_edgethreshold.GetInt()/2.0) ),1.0 ),0.0 );
			}

			m_iLastPosX = x;
			m_iLastPosY = y;
		}
	}

	// Clamp input speed
	cmd ->forwardmove = clamp( cmd ->forwardmove, -450.0f, 450.0f );
	cmd ->sidemove = clamp( cmd ->sidemove, -450.0f, 450.0f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2WarsInput::UpdateMouseRotation()
{
	int iMaxDelta = cl_strategic_cam_rot_maxdelta.GetInt();
	float rotation_direction = clamp(m_iDeltaX, -iMaxDelta, iMaxDelta);
	float rotation_direction_y = clamp(m_iDeltaY, -iMaxDelta, iMaxDelta);

	// Get view angles from engine
	QAngle	viewangles;	
	engine->GetViewAngles( viewangles );

	float fNewPitch = viewangles[PITCH] + (cl_strategic_cam_rot_speed.GetFloat() * rotation_direction_y * m_fCurrentSampleTime);
	if( fNewPitch > cl_strategic_cam_pitchdown.GetFloat() )
		fNewPitch = cl_strategic_cam_pitchdown.GetFloat();
	if( fNewPitch < -cl_strategic_cam_pitchup.GetFloat() )
		fNewPitch = -cl_strategic_cam_pitchup.GetFloat();

	viewangles[PITCH] = anglemod(fNewPitch);
	viewangles[YAW] = anglemod(viewangles[YAW] - (cl_strategic_cam_rot_speed.GetFloat() * rotation_direction * m_fCurrentSampleTime));
	engine->SetViewAngles( viewangles );
}

/*
================
AdjustAngles

Moves the local angle positions
================
*/
void CHL2WarsInput::AdjustAngles ( int nSlot, float frametime )
{
	C_HL2WarsPlayer *pPlayer = C_HL2WarsPlayer::GetLocalHL2WarsPlayer();
	if( !pPlayer || pPlayer->IsStrategicModeOn() == false )
	{
		CInput::AdjustAngles( nSlot, frametime );
		return;
	}

	float	speed;
	QAngle viewangles;
	
	// Determine control scaling factor ( multiplies time )
	speed = DetermineKeySpeed( nSlot, frametime );
	if ( speed <= 0.0f )
	{
		return;
	}

	// Retrieve latest view direction from engine
	engine->GetViewAngles( viewangles );

	// Undo tilting from previous frame
	viewangles -= GetPerUser().m_angPreviousViewAnglesTilt;

	// Apply tilting effects here (it affects aim)
	QAngle vecAnglesBeforeTilt = viewangles;
	GetViewEffects()->CalcTilt();
	GetViewEffects()->ApplyTilt( viewangles, 1.0f );

	// Remember the tilt delta so we can undo it before applying tilt next frame
	GetPerUser().m_angPreviousViewAnglesTilt = viewangles - vecAnglesBeforeTilt;

	// Adjust YAW
	AdjustYaw( nSlot, speed, viewangles );

	// Adjust PITCH if keyboard looking
	AdjustPitch( nSlot, speed, viewangles );
	
	// Make sure values are legitimate
	ClampAngles( viewangles );

	// Store new view angles into engine view direction
	engine->SetViewAngles( viewangles );
}

//-----------------------------------------------------------------------------
// Purpose: ClampAngles
//-----------------------------------------------------------------------------
void CHL2WarsInput::ClampAngles( QAngle& viewangles )
{
	if ( viewangles[PITCH] > cl_strategic_cam_pitchdown.GetFloat() )
	{
		viewangles[PITCH] = cl_strategic_cam_pitchdown.GetFloat();
	}
	if ( viewangles[PITCH] < -cl_strategic_cam_pitchup.GetFloat() )
	{
		viewangles[PITCH] = -cl_strategic_cam_pitchup.GetFloat();
	}

#ifndef PORTAL	// Don't constrain Roll in Portal because the player can be upside down! -Jeep
	if ( viewangles[ROLL] > 50 )
	{
		viewangles[ROLL] = 50;
	}
	if ( viewangles[ROLL] < -50 )
	{
		viewangles[ROLL] = -50;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Clamp mouse within screen
//-----------------------------------------------------------------------------
void CHL2WarsInput::ActivateMouseClipping( void )
{
#if 1
	engine->SetMouseWindowLock( true );
#else
	int x, y, w, h, offsetx, offsety;
	RECT r;
	POINT p;

	engine->GetScreenSize( w, h );

	// Calculate offset of clip rectangle
	vgui::input()->GetCursorPos( x, y );
	VCRHook_GetCursorPos( &p );

	offsetx = p.x - x;
	offsety = p.y - y;

	r.left = offsetx;
	r.right = offsetx + w;
	r.top = offsety;
	r.bottom = offsety + h;
	ClipCursor( &r );
#endif
}

void CHL2WarsInput::DeactivateMouseClipping( void )
{
#if 1
	engine->SetMouseWindowLock( false );
#else
	ClipCursor( NULL );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Hacky way to detect input focus.
//-----------------------------------------------------------------------------
BOOL CALLBACK FindClientWindow(HWND hwnd, LPARAM lParam)
{
	// Filter zero size windows. Don't care about those!
	RECT windowRect;
	GetWindowRect(hwnd, &windowRect);
	//Msg("\t%d Window size: %d %d %d %d\n", hwnd, windowRect.left, windowRect.top, windowRect.right, windowRect.bottom);
	if( windowRect.left == 0 && windowRect.top == 0 && windowRect.right == 0 && windowRect.bottom == 0 )
		return true;

	CHL2WarsInput *pInput = (CHL2WarsInput *)lParam;
	pInput->AddWindow( hwnd );
	return true;
}

void CHL2WarsInput::BuildWindowList()
{
	//Msg("START ENUM WINDOWS\n");
	m_Windows.RemoveAll();
	EnumThreadWindows( GetCurrentThreadId(), FindClientWindow, (LPARAM) this );

	// Just an informative warning
	if( m_Windows.Count() > 32 )
		DevMsg("HL2WarsInput::BuildWindowList: High number of windows associated with process! %d\n", m_Windows.Count() );
}

bool CHL2WarsInput::HasWindowFocus()
{
	int i;
	for( i = 0; i < m_Windows.Count(); i++ )
	{
		if( m_Windows[i] == ::GetFocus() )
			return true;
	}
	return false;
}
