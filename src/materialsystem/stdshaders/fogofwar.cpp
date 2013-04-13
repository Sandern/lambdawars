//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#include "BaseVSShader.h"

// Auto generated inc files
#include "fogofwar_vs30.inc"
#include "fogofwar_ps30.inc"

BEGIN_VS_SHADER( FogOfWar, "Help for FogOfWar" )

	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
	}

	SHADER_FALLBACK
	{
		return 0;
	}

	SHADER_INIT
	{
		LoadTexture( BASETEXTURE );
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			SetInitialShadowState();

			// Set stream format
			unsigned int flags = VERTEX_POSITION;
			int nTexCoordCount = 1;
			int userDataSize = 0;
			pShaderShadow->VertexShaderVertexFormat( flags, nTexCoordCount, NULL, userDataSize );

			// Vertex Shader
			DECLARE_STATIC_VERTEX_SHADER( fogofwar_vs30 );
			SET_STATIC_VERTEX_SHADER( fogofwar_vs30 );

			// Pixel Shader
			DECLARE_STATIC_PIXEL_SHADER( fogofwar_ps30 );
			SET_STATIC_PIXEL_SHADER_COMBO( FOW, true );
			SET_STATIC_PIXEL_SHADER( fogofwar_ps30 );

			// Textures
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			//pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, true );

			// Blending
			EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			pShaderShadow->EnableAlphaTest( true );
			pShaderShadow->AlphaFunc( SHADER_ALPHAFUNC_GREATER, 0.0f );
		}
		DYNAMIC_STATE
		{
			// Decide if this pass should be drawn
			static ConVarRef sv_fogofwar("sv_fogofwar");
			if( !sv_fogofwar.GetBool() )
			{
				Draw( false );
				return;
			}

			// Reset render state
			pShaderAPI->SetDefaultState();

			// Set Vertex Shader Combos
			DECLARE_DYNAMIC_VERTEX_SHADER( fogofwar_vs30 );
			SET_DYNAMIC_VERTEX_SHADER( fogofwar_vs30 );

			// Set Pixel Shader Combos
			DECLARE_DYNAMIC_PIXEL_SHADER( fogofwar_ps30 );
			SET_DYNAMIC_PIXEL_SHADER( fogofwar_ps30 );

			// Bind textures
			BindTexture( SHADER_SAMPLER0, BASETEXTURE );
		}
		Draw();
	}
END_SHADER
