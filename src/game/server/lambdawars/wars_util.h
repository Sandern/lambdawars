//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef WARS_UTIL_H
#define WARS_UTIL_H
#ifdef _WIN32
#pragma once
#endif

class CHL2WarsPlayer;

//-----------------------------------------------------------------------------
// Purpose: Tests if point is within camera limits of a player
//-----------------------------------------------------------------------------
inline bool TestPointInCamera( const Vector &vPoint, const Vector &vCamLimits, const matrix3x4_t &matAngles, const Vector &vPlayerPos )
{
	// Get point direction
	Vector vecToTarget = vPoint - vPlayerPos;
	vecToTarget.NormalizeInPlace();

	// Transform into player space
	VectorITransform( vecToTarget, matAngles, vecToTarget );

	// Test against camera angles of player
	if( vecToTarget.y > -vCamLimits.y && vecToTarget.y < vCamLimits.y &&
		vecToTarget.z > -vCamLimits.z && vecToTarget.z < vCamLimits.z )
	{
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Test if player can see the point
//-----------------------------------------------------------------------------
bool UTIL_Wars_InPlayerView( CHL2WarsPlayer *pPlayer, const Vector &origin );

#endif // WARS_UTIL_H