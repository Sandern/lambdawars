//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hl2wars_baseminimap.h"
#include <vgui/ISurface.h>
#include "c_hl2wars_player.h"
#include "hl2wars_util_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

// an overview map is 1024x1024 pixels
int CBaseMinimap::OVERVIEW_MAP_SIZE = 1024;

ConVar minimap_outline_alpha("minimap_outline_alpha", "220");

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseMinimap::CBaseMinimap(Panel *parent, const char *panelName, bool registerlisteners)
		: Panel(parent, panelName)
{
	maporigin = Vector( 0.0, 0.0, 0.0 );
	mapscale = 1.0f;

	maporigin = Vector( 0, 0, 0 );
	mapscale = 1.0f;
	zoom = 1.0f;
	mapcenter = Vector2D( 512, 512 );
	vieworigin = Vector2D( 512, 512 );
	viewangle = 0.0f;

	baserotation = 45.0f;
	followangle = false;
	rotatemap = false;

	playerviewcolor = Color(255, 255, 255, 220);

	if( registerlisteners )
	{
		ListenForGameEvent("game_newmap");
		ListenForGameEvent("round_start");
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseMinimap::FireGameEvent( IGameEvent *event )
{
	const char * type = event->GetName();

	if ( Q_strcmp(type, "game_newmap") == 0 )
	{
		SetMap( event->GetString("mapname") );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseMinimap::SetMap(const char * levelname)
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseMinimap::InsertEntityObject( CBaseEntity *pEnt, CHudTexture *pIcon, int iHalfWide, int iHalfTall )
{
	if( !pEnt ) {
		Warning("CBaseMinimap::InsertEntityObject: Entity is None\n");
		return;
	}
	m_EntityObjects.AddToTail( EntityObject(pEnt, pIcon, iHalfWide, iHalfTall) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseMinimap::RemoveEntityObject( CBaseEntity *pEnt )
{
	int i;
	for( i=0; i<m_EntityObjects.Count(); i++ )
	{
		if( m_EntityObjects.Element(i).m_hEntity == pEnt )
		{
			m_EntityObjects.Remove(i);
			break;
		}
	}
#ifdef ENABLE_PYTHON
	PyErr_SetString(PyExc_ValueError, "CBaseMinimap.RemoveEntityObject: x not in entity object list" );
	throw boost::python::error_already_set(); 
#endif // ENABLE_PYTHON
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseMinimap::DrawEntityObjects()
{
	int i, count;
	Vector2D pos2d;
	CBaseEntity *pEnt;
	Color color;

	count = m_EntityObjects.Count();


	for( i=count-1; i>=0; i-- )
	{
		EntityObject &eo = m_EntityObjects.Element(i);
		pEnt = eo.m_hEntity.Get();

		if( pEnt == NULL )
		{
			m_EntityObjects.Remove(i);
			continue;
		}

		surface()->DrawSetColor( 50, 50, 50, minimap_outline_alpha.GetInt() );
		if( !eo.m_pIcon )
		{
			pos2d = MapToPanel( WorldToMap( pEnt->GetAbsOrigin() ) );

			surface()->DrawOutlinedRect( int(pos2d.x-eo.m_iHalfWide-1), 
				int(pos2d.y-eo.m_iHalfTall-1), 
				int(pos2d.x+eo.m_iHalfWide+1), 
				int(pos2d.y+eo.m_iHalfTall+1)
			);		
		}

	}

	for( i=count-1; i>=0; i-- )
	{
		EntityObject &eo = m_EntityObjects.Element(i);
		pEnt = eo.m_hEntity.Get();

		if( pEnt == NULL )
		{
			m_EntityObjects.Remove(i);
			continue;
		}

		// Draw
		color.SetColor(pEnt->GetTeamColor().x*255, pEnt->GetTeamColor().y*255, pEnt->GetTeamColor().z*255, 255);
		pos2d = MapToPanel( WorldToMap( pEnt->GetAbsOrigin() ) );
		if( eo.m_pIcon )
		{
			if( eo.m_iHalfWide == 0 )
			{
				eo.m_pIcon->DrawSelf( pos2d.x - int(eo.m_pIcon->Width() / 2.0), pos2d.y - int(eo.m_pIcon->Height() / 2.0), color );
			}
			else
			{
				eo.m_pIcon->DrawSelf(	int(pos2d.x-eo.m_iHalfWide), 
										int(pos2d.y-eo.m_iHalfTall), 
									    eo.m_iHalfWide * 2,
										eo.m_iHalfTall * 2,
										color );
			}
		}
		else
		{
			surface()->DrawSetColor( color );
			surface()->DrawFilledRect( int(pos2d.x-eo.m_iHalfWide), 
				int(pos2d.y-eo.m_iHalfTall), 
				int(pos2d.x+eo.m_iHalfWide), 
				int(pos2d.y+eo.m_iHalfTall)
			);
		}

	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseMinimap::DrawPlayerView()
{
	C_HL2WarsPlayer *pPlayer = C_HL2WarsPlayer::GetLocalHL2WarsPlayer();
	if( !pPlayer )
		return;

	Vector start, forward1, forward2, point1, point2;
	Vector2D point2D1, point2D2, point2D3, point2D4;

	int mask = MASK_SOLID_BRUSHONLY; //CONTENTS_PLAYERCLIP;

	trace_t tr;
	//CTraceFilterWars traceFilter( pPlayer, COLLISION_GROUP_PLAYER_MOVEMENT );
	CTraceFilterWorldOnly traceFilter;

	start = pPlayer->Weapon_ShootPosition() + pPlayer->GetCameraOffset();
	forward1 = UTIL_GetMouseAim(pPlayer, 0, 0);
	forward2 = UTIL_GetMouseAim(pPlayer, ScreenWidth(), 0);

	

#if 0
	UTIL_TraceLine(start, pPlayer->Weapon_ShootPosition(), mask, &traceFilter, &tr);
	if( tr.fraction != 1.0f )
	{
		float fHeight = abs( start.z - pPlayer->Weapon_ShootPosition().z );

		point1 = start + forward1 * fHeight;
		point2 = start + forward2 * fHeight;
		Msg("Using default view box %f\n", fHeight);
	}
	else
#endif // 0
	{
		// Find the two lowest points
		UTIL_TraceLine(start, start + (forward1 *  8192), mask, &traceFilter, &tr);
		point1 = tr.endpos;

		UTIL_TraceLine(start, start + (forward2 *  8192), mask, &traceFilter, &tr);
		point2 = tr.endpos;
	}

    if( (start - point1).Length() > (start - point2).Length() )
	{
        point2D1 = MapToPanel( WorldToMap( start + forward1 * (start - point1).Length() ) );
        point2D2 = MapToPanel( WorldToMap( start + forward2 * (start - point1).Length() ) );
	}
    
	else
	{
        point2D1 = MapToPanel( WorldToMap( start + forward1 * (start - point2).Length() ) );
        point2D2 = MapToPanel( WorldToMap( start + forward2 * (start - point2).Length() ) );
	}

	// Find the two highest points
    forward1 = UTIL_GetMouseAim(pPlayer, ScreenWidth(), ScreenHeight() );
    UTIL_TraceLine(start, start + (forward1 *  8192), mask, &traceFilter, &tr);
    point1 = tr.endpos;

    forward2 = UTIL_GetMouseAim(pPlayer, 0, ScreenHeight()  );
    UTIL_TraceLine(start, start + (forward2 *  8192), mask, &traceFilter, &tr);
    point2 = tr.endpos;

    if( (start - point1).Length() > (start - point2).Length() )
	{
        point2D3 = MapToPanel( WorldToMap( start + forward1 * (start - point1).Length() ) );
        point2D4 = MapToPanel( WorldToMap( start + forward2 * (start - point1).Length() ) );
	}
    else
	{
        point2D3 = MapToPanel( WorldToMap( start + forward1 * (start - point2).Length() ) );
        point2D4 = MapToPanel( WorldToMap( start + forward2 * (start - point2).Length() ) );
	}

	// Draw them
	surface()->DrawSetColor( playerviewcolor );
	surface()->DrawLine( point2D1.x, point2D1.y, point2D2.x, point2D2.y );
	surface()->DrawLine( point2D2.x, point2D2.y, point2D3.x, point2D3.y );
	surface()->DrawLine( point2D3.x, point2D3.y, point2D4.x, point2D4.y );
	surface()->DrawLine( point2D4.x, point2D4.y, point2D1.x, point2D1.y );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CBaseMinimap::GetViewAngle( void )
{
	float flViewAngle = viewangle - (90.0f + baserotation); //135.0f;

	if ( !followangle )
	{
		// We don't use fViewAngle.  We just show straight at all times.
		if ( rotatemap )
			flViewAngle = 90.0f + baserotation; //135.0f;
		else
			flViewAngle = baserotation; //45.0f;
	}

	return flViewAngle;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector2D CBaseMinimap::WorldToMap( const Vector &worldpos )
{
	Vector2D offset( worldpos.x - maporigin.x, worldpos.y - maporigin.y);

	offset.x /=  mapscale;
	offset.y /= -mapscale;

	return offset;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector2D CBaseMinimap::MapToPanel( const Vector2D &mappos )
{
	int pwidth, pheight; 
	Vector2D panelpos;
	float viewAngle = GetViewAngle();

	GetSize(pwidth, pheight);

	Vector offset;
	offset.x = mappos.x - mapcenter.x;
	offset.y = mappos.y - mapcenter.y;
	offset.z = 0;

	VectorYawRotate( offset, viewAngle, offset );

	// find the actual zoom from the animationvar m_fZoom and the map zoom scale
	float fScale = (zoom * fullzoom) / OVERVIEW_MAP_SIZE;

	offset.x *= fScale;
	offset.y *= fScale;

	panelpos.x = (pwidth * 0.5f) + (pheight * offset.x);
	panelpos.y = (pheight * 0.5f) + (pheight * offset.y);

	return panelpos;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CBaseMinimap::MapToWorld( const Vector2D &offset )
{
	Vector worldpos;
	worldpos.x = offset.x*mapscale + maporigin.x;
	worldpos.y = offset.y*-mapscale + maporigin.y;
	return worldpos;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector2D CBaseMinimap::PanelToMap( const Vector2D &panelpos )
{	
	Vector2D mappos;
	Vector offset;
	int pwidth, pheight;
	float scale, viewAngle;

	viewAngle = -1.0 * GetViewAngle(); 

	GetSize(pwidth, pheight);

	offset.x = ( panelpos.x - ( pwidth * 0.5 ) ) / ( pheight );
	offset.y = ( panelpos.y - ( pheight * 0.5 ) ) / ( pheight );
	offset.z = 0.0;

	scale = zoom / OVERVIEW_MAP_SIZE;
	offset.x /= scale;
	offset.y /= scale;

	VectorYawRotate( offset, viewAngle, offset );

	mappos.x = offset.x + mapcenter.x;
	mappos.y = offset.y + mapcenter.y;

	return mappos;
}