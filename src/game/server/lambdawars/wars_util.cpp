//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "wars_util.h"
#include "hl2wars_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Simpler version of CServerNetworkProperty::IsInPVS
//-----------------------------------------------------------------------------
bool UTIL_Wars_InPlayerView( CHL2WarsPlayer *pPlayer, const Vector &origin )
{
	// Cull transmission based on the camera limits
	// These limits are send in cl_strategic_cam_limits (basically just the fov angles)
	// Get player camera position and limits
	Vector vPlayerPos = pPlayer->Weapon_ShootPosition() + pPlayer->GetCameraOffset();
	const Vector &vCamLimits = pPlayer->GetCamLimits();

	// Get player angles
	matrix3x4_t matAngles;
	AngleMatrix( pPlayer->GetAbsAngles(), matAngles );

	// Now check if the entity is within the camera limits
	return TestPointInCamera( origin, vCamLimits, matAngles, vPlayerPos );
}