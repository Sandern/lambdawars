//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//

/* Example how to plug this into an existing shader:

==================================================================================================== */

#include "BaseVSShader.h"
#include "mathlib/VMatrix.h"
#include "convar.h"
#include "fogofwar_blended_pass_helper.h"

// Auto generated inc files
#include "fogofwar_blended_pass_vs20.inc"
#include "fogofwar_blended_pass_ps20b.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/*
ConVar mat_fogofwar_r( "mat_fogofwar_r", "0.15" );
ConVar mat_fogofwar_g( "mat_fogofwar_g", "0.15" );
ConVar mat_fogofwar_b( "mat_fogofwar_b", "0.15" );
ConVar mat_fogofwar_a( "mat_fogofwar_a", "0.5" );
*/

void InitParamsFogOfWarBlendedPass( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, FogOfWarBlendedPassVars_t &info )
{
	SET_PARAM_STRING_IF_NOT_DEFINED( info.m_nFogOfWarTexture, "__rt_fow" );
}

void InitFogOfWarBlendedPass( CBaseVSShader *pShader, IMaterialVar** params, FogOfWarBlendedPassVars_t &info )
{
	// Load textures
	pShader->LoadTexture( info.m_nBaseTexture );
	pShader->LoadTexture( info.m_nFogOfWarTexture );
}

void DrawFogOfWarBlendedPass( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
								  IShaderShadow* pShaderShadow, FogOfWarBlendedPassVars_t &info, VertexCompressionType_t vertexCompression )
{
	bool bVertexLitGeneric = false;
	bool bHasFlashlight = false;

	SHADOW_STATE
	{
		bool hasBaseAlphaEnvmapMask = IS_FLAG_SET( MATERIAL_VAR_BASEALPHAENVMAPMASK );
		bool bHasSelfIllum = (!bHasFlashlight || IsX360() ) && IS_FLAG_SET( MATERIAL_VAR_SELFILLUM );
		bool bIsAlphaTested = IS_FLAG_SET( MATERIAL_VAR_ALPHATEST ) != 0;

		// Reset shadow state manually since we're drawing from two materials
		pShader->SetInitialShadowState();

		// Set stream format (note that this shader supports compression)
		unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_FORMAT_COMPRESSED;
		int nTexCoordCount = 1;
		int userDataSize = 0;
		pShaderShadow->VertexShaderVertexFormat( flags, nTexCoordCount, NULL, userDataSize );

		// Vertex Shader
		DECLARE_STATIC_VERTEX_SHADER( fogofwar_blended_pass_vs20 );
		SET_STATIC_VERTEX_SHADER_COMBO( FOW, true );
		SET_STATIC_VERTEX_SHADER( fogofwar_blended_pass_vs20 );

		// Pixel Shader
		DECLARE_STATIC_PIXEL_SHADER( fogofwar_blended_pass_ps20b );
		SET_STATIC_PIXEL_SHADER_COMBO( BASEALPHAENVMAPMASK,  hasBaseAlphaEnvmapMask );
		SET_STATIC_PIXEL_SHADER_COMBO( SELFILLUM,  bHasSelfIllum );
		SET_STATIC_PIXEL_SHADER_COMBO( FOW, true );
		SET_STATIC_PIXEL_SHADER( fogofwar_blended_pass_ps20b );

		pShader->DefaultFog();

		// Textures
		pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
		pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
		//pShaderShadow->EnableSRGBRead( SHADER_SAMPLER1, true );

		// Blending
		pShader->EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
		pShaderShadow->EnableAlphaTest( bIsAlphaTested );
		pShaderShadow->AlphaFunc( SHADER_ALPHAFUNC_GREATER, 0.0f );
	}
	DYNAMIC_STATE
	{
		// Decide if this pass should be drawn
		static ConVarRef sv_fogofwar("sv_fogofwar");
		//static ConVarRef sv_fogofwar_tilesize("sv_fogofwar_tilesize");
		if( !sv_fogofwar.GetBool() )
		{
			pShader->Draw( false );
			return;
		}

		// Reset render state manually since we're drawing from two materials
		pShaderAPI->SetDefaultState();

		// Set Vertex Shader Combos
		DECLARE_DYNAMIC_VERTEX_SHADER( fogofwar_blended_pass_vs20 );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( DOWATERFOG, pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
		SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, pShaderAPI->GetCurrentNumBones() > 0 );
		SET_DYNAMIC_VERTEX_SHADER( fogofwar_blended_pass_vs20 );

		// Set Vertex Shader Constants 
		//pShader->SetAmbientCubeDynamicStateVertexShader();

		// Set Pixel Shader Combos
		DECLARE_DYNAMIC_PIXEL_SHADER( fogofwar_blended_pass_ps20b );
		SET_DYNAMIC_PIXEL_SHADER( fogofwar_blended_pass_ps20b );

		// Bind textures
		pShader->BindTexture( SHADER_SAMPLER0, info.m_nBaseTexture );
		pShader->BindTexture( SHADER_SAMPLER1, info.m_nFogOfWarTexture );

		// Set Pixel Shader Constants 
		//pShader->SetModulationPixelShaderDynamicState_LinearColorSpace( 1 );

		float eyePos[4];
		pShaderAPI->GetWorldSpaceCameraPosition( eyePos );
		pShaderAPI->SetPixelShaderConstant( 0, eyePos, 1 );

		bool bWriteDepthToAlpha = pShaderAPI->ShouldWriteDepthToDestAlpha();
		bool bWriteWaterFogToAlpha = false;
		bool bHasVertexAlpha =  bVertexLitGeneric ? false : IS_FLAG_SET( MATERIAL_VAR_VERTEXALPHA );

		float fPixelFogType = pShaderAPI->GetPixelFogCombo() == 1 ? 1 : 0;
		float fWriteDepthToAlpha = bWriteDepthToAlpha && IsPC() ? 1 : 0;
		float fWriteWaterFogToDestAlpha = bWriteWaterFogToAlpha ? 1 : 0;
		float fVertexAlpha = bHasVertexAlpha ? 1 : 0;

		// Controls for lerp-style paths through shader code (bump and non-bump have use different register)
		float vShaderControls[4] = { fPixelFogType, fWriteDepthToAlpha, fWriteWaterFogToDestAlpha, fVertexAlpha	 };
		pShaderAPI->SetPixelShaderConstant( 1, vShaderControls, 1 );

		pShaderAPI->SetPixelShaderFogParams(2);


		float	vFoWSize[ 4 ];
		Vector	vMins = pShaderAPI->GetVectorRenderingParameter( VECTOR_RENDERPARM_GLOBAL_FOW_MINS );
		Vector	vMaxs = pShaderAPI->GetVectorRenderingParameter( VECTOR_RENDERPARM_GLOBAL_FOW_MAXS );
		vFoWSize[ 0 ] = vMins.x;
		vFoWSize[ 1 ] = vMins.y;
		vFoWSize[ 2 ] = vMaxs.x - vMins.x;
		vFoWSize[ 3 ] = vMaxs.y - vMins.y;
		pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_3, vFoWSize );
		/*
		// Fog of war color
		static float c[4];
		c[0] = mat_fogofwar_r.GetFloat();
		c[1] = mat_fogofwar_g.GetFloat();
		c[2] = mat_fogofwar_b.GetFloat();
		c[3] = mat_fogofwar_a.GetFloat();
		pShaderAPI->SetPixelShaderConstant( 3, (const float *)(&c), 1 );
		*/

		// Tilesize
		//static float ts[4];
		//ts[0] = sv_fogofwar_tilesize.GetInt();
		//pShaderAPI->SetPixelShaderConstant( 4, (const float *)(&ts), 1 );
	}
	pShader->Draw();
}