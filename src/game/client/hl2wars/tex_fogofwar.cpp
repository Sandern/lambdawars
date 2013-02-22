//====== Copyright © 2013 Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hl2wars/tex_fogofwar.h"
#include "materialsystem/IMaterialVar.h"
#include "hl2wars/fowmgr.h"
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// convars
extern ConVar sv_fogofwar;
extern ConVar cl_fogofwar_noconverge;

int g_bFOWUpdateType = TEXFOW_NONE;

//-----------------------------------------------------------------------------
// Regenerate the texture
//-----------------------------------------------------------------------------
void CFOWTextureRegen::RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pSubRect )
{
	// Do not update except in the FOW Manager class.
	// Otherwise it might give an incorrect representation of the fog of war.
	if( g_bFOWUpdateType == TEXFOW_NONE )
		return;

	VPROF_BUDGET( "CFOWTextureRegen::RegenerateTextureBits", VPROF_BUDGETGROUP_FOGOFWAR );

	int nGridSize = FogOfWarMgr()->m_nGridSize;
	unsigned char *imageData = pVTFTexture->ImageData( 0, 0, 0 );
	unsigned char *pFowData;
	if( g_bFOWUpdateType == TEXFOW_DIRECT )
		pFowData = FogOfWarMgr()->m_FogOfWar.Base();
	else
		pFowData = FogOfWarMgr()->m_FogOfWarTextureData.Base();

	if( !pSubRect || pSubRect->width == nGridSize )
	{
		//Msg("FULL FOW UPDATE TEXTURE DOWNLOAD\n");
		Q_memcpy( imageData, FogOfWarMgr()->m_FogOfWar.Base(), nGridSize*nGridSize );
	}
	else
	{
		//engine->Con_NPrintf(2, "Rect: %d %d %d %d", pSubRect->x, pSubRect->y, pSubRect->width, pSubRect->height);
		for( int y = pSubRect->y; y <  pSubRect->y+pSubRect->height; y++ )
		{
			Q_memcpy( imageData + (y * nGridSize) + (pSubRect->x), // Our current row + x offset
				pFowData + (y * nGridSize) + (pSubRect->x), // Source offset
				pSubRect->width // size of row we want to copy
			); 
		}
	}
}
