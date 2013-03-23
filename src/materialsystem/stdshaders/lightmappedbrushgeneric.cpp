//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: Multiblending shader for Spy in TF2 (and probably many other things to come)
//
// $NoKeywords: $
//=====================================================================================//

#include "BaseVSShader.h"
#include "convar.h"
#include "lightmappedbrushgeneric_dx9_helper.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"


DEFINE_FALLBACK_SHADER( LightmappedBrushGeneric, LightmappedBrushGeneric_DX90 )

BEGIN_VS_SHADER( LightmappedBrushGeneric_DX90, "Help for LightmappedBrushGeneric" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( FOW, SHADER_PARAM_TYPE_TEXTURE, "_rt_fog_of_war", "FoW Render Target" )
		SHADER_PARAM( TEAMCOLOR, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Team color" )
		SHADER_PARAM( TEAMCOLORMAP, SHADER_PARAM_TYPE_TEXTURE, "", "Texture describing which places should be team colored." )
	END_SHADER_PARAMS

	void SetupVars( LightmappedBrushGeneric_DX9_Vars_t& info )
	{
		info.m_nBaseTexture = BASETEXTURE;
		info.m_nFoW = FOW;
		info.m_nTeamColor = TEAMCOLOR;
		info.m_nTeamColorTexture = TEAMCOLORMAP;
	}

	SHADER_INIT_PARAMS()
	{
		LightmappedBrushGeneric_DX9_Vars_t info;
		SetupVars( info );
		InitParamsLightmappedBrushGeneric_DX9( this, params, pMaterialName, info );
	}

	SHADER_FALLBACK
	{
		return 0;
	}

	SHADER_INIT
	{
		LightmappedBrushGeneric_DX9_Vars_t info;
		SetupVars( info );
		InitLightmappedBrushGeneric_DX9( this, params, info );
	}

	SHADER_DRAW
	{
		LightmappedBrushGeneric_DX9_Vars_t info;
		SetupVars( info );
		DrawLightmappedBrushGeneric_DX9( this, params, pShaderAPI, pShaderShadow, info, vertexCompression, pContextDataPtr );
	}
END_SHADER

