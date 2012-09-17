//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "hl2wars_util_shared.h"
#include "wars_mapboundary.h"

#ifdef GAME_DLL
	#include "nav_mesh.h"
	#include "Sprite.h"
#ifndef DISABLE_PYTHON
	#include "src_python_navmesh.h"
#endif // DISABLE_PYTHON
#else
	#include "c_hl2wars_player.h"
#endif // GAME_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef HL2WARS_ASW_DLL
#ifdef GAME_DLL
// link clientside sprite to CSprite - can remove this if we fixup all the maps to not have _clientside ones (do when we're sure we don't need clientside sprites)
LINK_ENTITY_TO_CLASS( env_sprite_clientside, CSprite );
#endif
#endif // HL2WARS_ASW_DLL

void UTIL_FindPositionInRadius( positioninradius_t &info )
{
	Vector vecDir, vecTest;
	trace_t tr;
	for(; info.m_vFan.y < 360; info.m_vFan.y += info.m_fStepSize )
	{
		AngleVectors( info.m_vFan, &vecDir );

		vecTest = info.m_vStartPosition + vecDir * info.m_fRadius;

		Vector vEndPos;
#ifdef GAME_DLL
		if( TheNavMesh->IsLoaded() )
		{
#ifndef DISABLE_PYTHON
			vEndPos = NavMeshGetPositionNearestNavArea(vecTest, info.m_fBeneathLimit);
#else
			vEndPos = vec3_origin;
#endif // DISABLE_PYTHON
            if( vEndPos == vec3_origin )
                continue;
            vEndPos.z += 32.0;
		}
		else
#endif // GAME_DLL
		{
			UTIL_TraceLine( vecTest, vecTest - Vector( 0, 0, 8192 ), MASK_SHOT, info.m_hIgnore.Get(), COLLISION_GROUP_NONE, &tr );
			if( tr.fraction == 1.0 )
				continue;
			vEndPos = tr.endpos;
		}
            
        UTIL_TraceHull( vEndPos,
                        vEndPos + Vector( 0, 0, 10 ),
                        info.m_vMins,
                        info.m_vMaxs,
                        MASK_NPCSOLID,
                        info.m_hIgnore.Get(),
                        COLLISION_GROUP_NONE,
                        &tr );

        if( tr.fraction == 1.0 )
            info.m_vPosition = tr.endpos;
            info.m_bSuccess = true;
            info.m_vFan.y += info.m_fStepSize;
            return;     
	}

	info.m_bSuccess = false;
}

void UTIL_FindPosition( positioninfo_t &info )
{
	trace_t tr;
	if( info.m_fRadius == 0 )
	{
		// Find a start pos
		Vector vEndPos;
#ifdef GAME_DLL
		if( TheNavMesh->IsLoaded() )
		{
#ifndef DISABLE_PYTHON
			vEndPos = NavMeshGetPositionNearestNavArea(info.m_vPosition, info.m_fBeneathLimit);
#else
			vEndPos= vec3_origin;
#endif // DISABLE_PYTHON
            if( vEndPos != vec3_origin )
				vEndPos.z += 32.0;
		}
		else
#endif // GAME_DLL
		{
			UTIL_TraceLine( info.m_vPosition, info.m_vPosition - Vector( 0, 0, 8192 ), MASK_SHOT, info.m_hIgnore.Get(), COLLISION_GROUP_NONE, &tr );
			vEndPos = ( tr.fraction == 1.0 ) ? tr.endpos : vec3_origin;
		}

		// Test pos
        UTIL_TraceHull( info.m_vPosition,
                        info.m_vPosition + Vector( 0, 0, 10 ),
                        info.m_vMins,
                        info.m_vMaxs,
                        MASK_NPCSOLID,
                        info.m_hIgnore,
                        COLLISION_GROUP_NONE,
                        &tr );
        info.m_fRadius += info.m_fRadiusStep;
        if( tr.fraction == 1.0 )
		{
            info.m_vPosition = tr.endpos;
            info.m_bSuccess = true;
            return;
		}
	}

	while( info.m_fRadius <= info.m_fMaxRadius )
	{
		info.m_InRadiusInfo.m_fRadius = info.m_fRadius;
		info.m_InRadiusInfo.ComputeRadiusStepSize();

        UTIL_FindPositionInRadius(info.m_InRadiusInfo);
        if( info.m_InRadiusInfo.m_bSuccess )
		{
            info.m_vPosition = info.m_InRadiusInfo.m_vPosition;
            info.m_bSuccess = true;
            return;
		}
        info.m_InRadiusInfo.m_vFan.Init(0,0,0);
        info.m_fRadius += info.m_fRadiusStep;
	}
    info.m_bSuccess = false;
}

#ifdef CLIENT_DLL
Vector UTIL_GetMouseAim( C_HL2WarsPlayer *pPlayer, int x, int y )
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
	screenToWorld.V3Mul( v1, pPlayer->Weapon_ShootPosition() ); // ray start origin 
	screenToWorld.V3Mul( v2, o2 ); // ray end origin 
	VectorSubtract( o2, pPlayer->Weapon_ShootPosition(), forward ); 
	forward.NormalizeInPlace(); 
	return forward;
}

inline void TestPoints( const Vector &vTestPoint, int &minsx, int &minsy, int &maxsx, int &maxsy )
{
	int tempx, tempy;
	GetVectorInScreenSpace(vTestPoint, tempx, tempy); 
	if( tempx < minsx )
		minsx = tempx;
	if( tempx > maxsx )
		maxsx = tempx;
	if( tempy < minsy )
		minsy = tempy;
	if( tempy > maxsy )
		maxsy = tempy;
}

void UTIL_GetScreenMinsMaxs( const Vector &origin, const QAngle &angle, const Vector &mins, const Vector &maxs, 
							int &minsx, int &minsy, int &maxsx, int &maxsy )
{
	Vector vTestPoint, vTransformedMins, vTransformedMaxs;
	matrix3x4_t xform;

	// Transform
    AngleMatrix( angle, origin, xform );
    TransformAABB( xform, mins, maxs, vTransformedMins, vTransformedMaxs );

	//NDebugOverlay::BoxAngles( origin, mins, maxs, angle, 255, 0, 0, 200, 0.1 );
	NDebugOverlay::Cross3D( vTransformedMins, -Vector(16, 16, 16), Vector(16, 16, 16), 255, 0, 0, false, 0.1 );
	NDebugOverlay::Cross3D( vTransformedMaxs, -Vector(16, 16, 16), Vector(16, 16, 16), 0, 255, 0, false, 0.1 );

	// Init points
	minsx = INT_MAX;
	maxsx = -1;
	minsy = INT_MAX;
	maxsy = -1;

	// Test points
	// find the smallest and biggest values
	TestPoints( vTransformedMins, minsx, minsy, maxsx, maxsy ); 
	TestPoints( vTransformedMaxs, minsx, minsy, maxsx, maxsy );

	/*TestPoints( Vector(vTransformedMins.x, vTransformedMins.y, vTransformedMaxs.z), minsx, minsy, maxsx, maxsy ); 
	TestPoints( Vector(vTransformedMaxs.x, vTransformedMins.y, vTransformedMaxs.z), minsx, minsy, maxsx, maxsy ); 
	TestPoints( Vector(vTransformedMins.x, vTransformedMaxs.y, vTransformedMaxs.z), minsx, minsy, maxsx, maxsy ); */

	TestPoints( Vector(vTransformedMaxs.x, vTransformedMaxs.y, vTransformedMins.z), minsx, minsy, maxsx, maxsy ); 
	TestPoints( Vector(vTransformedMaxs.x, vTransformedMins.y, vTransformedMins.z), minsx, minsy, maxsx, maxsy ); 
	TestPoints( Vector(vTransformedMins.x, vTransformedMaxs.y, vTransformedMins.z), minsx, minsy, maxsx, maxsy );
}

#endif // CLIENT_DLL


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTraceFilterSkipFriendly::CTraceFilterSkipFriendly( const IHandleEntity *passentity, 
												   int collisionGroup, CUnitBase *pUnit )
	: CTraceFilterSimple( passentity, collisionGroup ), m_pUnit(pUnit)
{
}

bool CTraceFilterSkipFriendly::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
	if ( !pEntity )
		return false;

	CUnitBase *pUnit = pEntity->MyUnitPointer();
	if( m_pUnit && pUnit && m_pUnit->IRelationType(pUnit) == D_LI )
	{
		return false;
	}

	return CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask );
}

CTraceFilterSkipEnemies::CTraceFilterSkipEnemies( const IHandleEntity *passentity, int collisionGroup, CUnitBase *pUnit )
	: CTraceFilterSimple( passentity, collisionGroup ), m_pUnit(pUnit)
{
}

bool CTraceFilterSkipEnemies::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
	if ( !pEntity )
		return false;

	CUnitBase *pUnit = pEntity->MyUnitPointer();
	if( m_pUnit && pUnit && m_pUnit->IRelationType(pUnit) == D_HT )
	{
		return false;
	}

	return CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask );
}

bool CTraceFilterWars::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	// Only collide with the map boundaries!
	CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
	if ( !pEntity || !(dynamic_cast<CBaseFuncMapBoundary *>(pEntity)) )
		return false;
	return true;
}