//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hl2wars_in_main.h"
#include "c_hl2wars_player.h"
#include "wars_mapboundary.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar cam_command;
extern ConVar cam_showangles;

ConVar cl_strategic_cam_spring_vel_max( "cl_strategic_cam_spring_vel_max", "3000.0", FCVAR_ARCHIVE, "Camera max velocity." );
ConVar cl_strategic_cam_spring_const( "cl_strategic_cam_spring_const", "49", FCVAR_ARCHIVE, "Camera spring constant." );
ConVar cl_strategic_cam_spring_dampening( "cl_strategic_cam_spring_dampening", "3.0", FCVAR_ARCHIVE, "Camera spring dampening." );
extern ConVar cl_strategic_height_tol;

ConVar cl_strategic_cam_min_dist( "cl_strategic_cam_min_dist", "240.0", FCVAR_CHEAT, "Camera minimum distance" );
ConVar cl_strategic_cam_max_dist( "cl_strategic_cam_max_dist", "840.0", FCVAR_CHEAT, "Camera maximum distance" );

#define	DIST	 2

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CHL2WarsInput::WARS_GetCameraPitch( )
{
	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CHL2WarsInput::WARS_GetCameraYaw( )
{
	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CHL2WarsInput::WARS_GetCameraDist( )
{
	C_HL2WarsPlayer *pPlayer = C_HL2WarsPlayer::GetLocalHL2WarsPlayer();
	if( !pPlayer )
	{
		return 0.0f;
	}

	// No map boundary? No distance (act like firstperson).
	if( !GetMapBoundaryList() )
	{
		return 0.0f;
	}

	if( m_fLastOriginZ != -1 )
	{
		float fDiff = pPlayer->GetAbsOrigin().z - m_fLastOriginZ;
		if( fabs(fDiff) > 2.0f )
		{
			m_flCurrentCameraDist -= fDiff;
		}
	}
	m_fLastOriginZ = pPlayer->GetAbsOrigin().z;

	// Single player doesn't run client side movement, so update height here
	if( gpGlobals->maxClients == 1 )
	{
		pPlayer->CalculateHeight( pPlayer->GetAbsOrigin() );
	}

	// Reset if desired height or max height is not yet set
	if( m_flDesiredCameraDist == -1 )
	{
		pPlayer->CalculateHeight( pPlayer->GetAbsOrigin() ); // Ensure we have a valid height
		m_flDesiredCameraDist = m_flCurrentCameraDist = clamp( 800.0f, cl_strategic_cam_min_dist.GetFloat(), MIN( cl_strategic_cam_max_dist.GetFloat(), pPlayer->GetCamMaxHeight() ) );
		m_flCurrentCameraDist = clamp( m_flCurrentCameraDist, cl_strategic_cam_min_dist.GetFloat(), pPlayer->GetCamMaxHeight() );
	}

	// Clamp camera height within the valid range
	if( pPlayer->GetCamMaxHeight() != -1 )
	{
		m_flDesiredCameraDist = clamp( m_flDesiredCameraDist, cl_strategic_cam_min_dist.GetFloat(), MIN( cl_strategic_cam_max_dist.GetFloat(), pPlayer->GetCamMaxHeight() ) );
	}

	// Converge cam height
	float flCameraDelta = fabs( m_flCurrentCameraDist - m_flDesiredCameraDist );

	// Check against a tolerance so we don't oscillate forever.
	if ( flCameraDelta > cl_strategic_height_tol.GetFloat() )
	{
		// Get frametime = delta time
		float flFrameTime = gpGlobals->frametime;

		float flAccel = fabs( m_flDesiredCameraDist - m_flCurrentCameraDist ) * cl_strategic_cam_spring_const.GetFloat();
		float flZDampening = cl_strategic_cam_spring_dampening.GetFloat() * m_vecCameraVelocity.z;
		flAccel -= flZDampening;
		float flZVelocity = flAccel * flFrameTime;
		m_vecCameraVelocity.z += flZVelocity;
		m_vecCameraVelocity.z = clamp( m_vecCameraVelocity.z, 0.1f, cl_strategic_cam_spring_vel_max.GetFloat() );
		if ( m_flCurrentCameraDist < m_flDesiredCameraDist )
		{
			m_flCurrentCameraDist = MIN( m_flCurrentCameraDist + m_vecCameraVelocity.z * flFrameTime, m_flDesiredCameraDist );
		}
		else
		{
			m_flCurrentCameraDist = MAX( m_flCurrentCameraDist - m_vecCameraVelocity.z * flFrameTime, m_flDesiredCameraDist );
		}
	}
	else
	{
		m_vecCameraVelocity.z = 0.0f;
		m_flCurrentCameraDist = m_flDesiredCameraDist;
	}

	return m_flCurrentCameraDist;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2WarsInput::CAM_Think( void )
{
	QAngle viewangles;

	C_HL2WarsPlayer *pPlayer = C_HL2WarsPlayer::GetLocalHL2WarsPlayer();
	if( !pPlayer || pPlayer->GetControlledUnit() )
	{
		CInput::CAM_Think();
		return;
	}
	
	PerUserInput_t &user = GetPerUser();

	switch( user.m_nCamCommand )
	{
	case CAM_COMMAND_TOTHIRDPERSON:
		CAM_ToThirdPerson();
		break;
		
	case CAM_COMMAND_TOFIRSTPERSON:
		CAM_ToFirstPerson();
		break;
		
	case CAM_COMMAND_NONE:
	default:
		break;
	}
	
	if( !user.m_fCameraInThirdPerson )
		return;

	engine->GetViewAngles( viewangles );

	GetPerUser().m_vecCameraOffset[ PITCH ] = WARS_GetCameraPitch() + viewangles.x;
	GetPerUser().m_vecCameraOffset[ YAW ]   = WARS_GetCameraYaw() + viewangles.y;
	GetPerUser().m_vecCameraOffset[ DIST ]  = WARS_GetCameraDist();
}

/*
==============================
CAM_GetCameraOffset

==============================
*/
void CHL2WarsInput::CAM_GetCameraOffset( Vector& ofs )
{
	CInput::CAM_GetCameraOffset(ofs);
	/*C_HL2WarsPlayer *pPlayer = C_HL2WarsPlayer::GetLocalHL2WarsPlayer();
	if( !pPlayer || pPlayer->IsStrategicModeOn() == false) {
		CInput::CAM_GetCameraOffset(ofs);
		return;
	}
	VectorCopy( m_vecCameraOffset, ofs );
	*/
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2WarsInput::Wars_CamReset()
{
	QAngle viewangles;
	engine->GetViewAngles( viewangles );
	viewangles[PITCH] = 65;
	viewangles[YAW] = 135;
	engine->SetViewAngles( viewangles );
}



CON_COMMAND_F( cam_reset, "Reset camera", 0 )
{
	((CHL2WarsInput *)input)->Wars_CamReset();
}

CON_COMMAND_F( cam_debug_info, "Debug camera info", 0 )
{
	CHL2WarsInput *wars_input = (CHL2WarsInput *)input;
	if( !wars_input )
		return;

	C_HL2WarsPlayer *pPlayer = C_HL2WarsPlayer::GetLocalHL2WarsPlayer();
	if( !pPlayer )
		return;

	Msg("Player Position: %f %f %f\n", pPlayer->GetAbsOrigin().x, pPlayer->GetAbsOrigin().y, pPlayer->GetAbsOrigin().z );
	Msg("Player Max Cam Height: %f\n", pPlayer->GetCamMaxHeight() );
	Msg("Current Cam Height: %f\n", wars_input->GetCurrentCamHeight() );
	Msg("Desired Cam Height: %f\n", wars_input->GetDesiredCamHeight() );
}