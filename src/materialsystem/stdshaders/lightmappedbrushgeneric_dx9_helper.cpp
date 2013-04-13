//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "BaseVSShader.h"
#include "shaderlib/commandbuilder.h"
#include "lightmappedbrushgeneric_dx9_helper.h"
#include "..\shaderapidx9\locald3dtypes.h"												   
#include "convar.h"
#include "cpp_shader_constant_register_map.h"

#include "lightmappedbrushgeneric_vs30.inc"
#include "lightmappedbrushgeneric_ps30.inc"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"


// FIXME: doesn't support fresnel!
void InitParamsLightmappedBrushGeneric_DX9( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, LightmappedBrushGeneric_DX9_Vars_t &info )
{
	SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
	if( g_pConfig->UseBumpmapping() && params[info.m_nBumpmap]->IsDefined() )
	{
		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_BUMPED_LIGHTMAP );
	}

	if( !params[info.m_nBumpFrame]->IsDefined() )
		params[info.m_nBumpFrame]->SetIntValue( 0 );

	if( !params[info.m_nEnvmapFrame]->IsDefined() )
		params[info.m_nEnvmapFrame]->SetIntValue( 0 );

	if( !params[info.m_nEnvmapTint]->IsDefined() )
		params[info.m_nEnvmapTint]->SetVecValue( 1.0f, 1.0f, 1.0f );

	params[FLASHLIGHTTEXTURE]->SetStringValue( GetFlashlightTextureFilename() );

	SET_PARAM_STRING_IF_NOT_DEFINED( info.m_nFoW, "_rt_fog_of_war" );
}

void InitLightmappedBrushGeneric_DX9( CBaseVSShader *pShader, IMaterialVar** params, LightmappedBrushGeneric_DX9_Vars_t &info )
{
	if ( params[ info.m_nBaseTexture ]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nBaseTexture );
	}

	if ( g_pConfig->UseBumpmapping() && params[info.m_nBumpmap]->IsDefined() )
	{
		pShader->LoadBumpMap( info.m_nBumpmap );
	}

	if ( g_pConfig->UseSpecular() )
	{
		if ( params[info.m_nEnvmap]->IsDefined() )
		{
			pShader->LoadCubeMap( info.m_nEnvmap );
		}
	}
	else
	{
		params[info.m_nEnvmap]->SetUndefined();
	}

	if ( info.m_nFoW != -1 && params[ info.m_nFoW ]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nFoW );
	}

	if( (info.m_nTeamColorTexture != -1) && params[info.m_nTeamColorTexture]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nTeamColorTexture );
	}
}

void DrawLightmappedBrushGeneric_DX9( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
				    IShaderShadow* pShaderShadow, LightmappedBrushGeneric_DX9_Vars_t &info, VertexCompressionType_t vertexCompression,
					CBasePerMaterialContextData **pContextDataPtr )
{
	bool bHasFoW = ( ( info.m_nFoW != -1 ) && ( params[ info.m_nFoW ]->IsTexture() != 0 ) );
	if ( bHasFoW == true )
	{
		ITexture *pTexture = params[ info.m_nFoW ]->GetTextureValue();
		if ( ( pTexture->GetFlags() & TEXTUREFLAGS_RENDERTARGET ) == 0 )
		{
			bHasFoW = false;
		}
	}
	int nLightingPreviewMode = IS_FLAG2_SET( MATERIAL_VAR2_USE_GBUFFER0 ) + 2 * IS_FLAG2_SET( MATERIAL_VAR2_USE_GBUFFER1 );

	bool bHasTeamColorTexture = ( info.m_nTeamColorTexture != -1 ) && params[info.m_nTeamColorTexture]->IsTexture();
	bool bHasEnvmap = info.m_nEnvmap != -1 && params[info.m_nEnvmap]->IsTexture();
	bool bHasBump = ( params[info.m_nBumpmap]->IsTexture() ) && g_pConfig->UseBumpmapping();

	SHADOW_STATE
	{
		pShader->SetInitialShadowState( );

		pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, true );	// Always SRGB read on base map 1

		// Lightmap sampler
		pShaderShadow->EnableTexture( SHADER_SAMPLER5, true );
		if( g_pHardwareConfig->GetHDRType() == HDR_TYPE_NONE )
		{
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER5, true );
		}
		else
		{
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER5, false );
		}

		pShaderShadow->EnableSRGBWrite( true );
		pShaderShadow->EnableAlphaWrites( true ); // writing water fog alpha always.

		unsigned int flags = VERTEX_POSITION;

		if ( bHasFoW )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER10, true );
		}

		if( bHasTeamColorTexture )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
		}

		if( bHasBump )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );
		}

		if( bHasEnvmap )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER4, true );
			if( g_pHardwareConfig->GetHDRType() == HDR_TYPE_NONE )
			{
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER4, true );
			}

			flags |= VERTEX_TANGENT_S | VERTEX_TANGENT_T | VERTEX_NORMAL;
		}

		int nTexCoordCount = bHasBump ? 3 : 2;

		pShaderShadow->VertexShaderVertexFormat( flags, nTexCoordCount, 0, 0 );

		// The vertex shader uses the vertex id stream
		SET_FLAGS2( MATERIAL_VAR2_USES_VERTEXID );

		DECLARE_STATIC_VERTEX_SHADER( lightmappedbrushgeneric_vs30 );
		SET_STATIC_VERTEX_SHADER_COMBO( FOW, bHasFoW );
		SET_STATIC_VERTEX_SHADER( lightmappedbrushgeneric_vs30 );

		// Bind ps_2_b shader so we can get Phong terms
		DECLARE_STATIC_PIXEL_SHADER( lightmappedbrushgeneric_ps30 );
		SET_STATIC_PIXEL_SHADER_COMBO( BUMPMAP, bHasBump );
		SET_STATIC_PIXEL_SHADER_COMBO( CUBEMAP, bHasEnvmap );
		SET_STATIC_PIXEL_SHADER_COMBO( LIGHTING_PREVIEW, nLightingPreviewMode );
		SET_STATIC_PIXEL_SHADER_COMBO( FOW, bHasFoW );
		SET_STATIC_PIXEL_SHADER_COMBO( TEAMCOLORTEXTURE, bHasTeamColorTexture );
		SET_STATIC_PIXEL_SHADER( lightmappedbrushgeneric_ps30 );

		pShader->DefaultFog();

		// Lighting constants
		float flLScale = pShaderShadow->GetLightMapScaleFactor();
		pShader->PI_BeginCommandBuffer();
		pShader->PI_SetPixelShaderAmbientLightCube( PSREG_AMBIENT_CUBE );
		//pShader->PI_SetModulationPixelShaderDynamicState( 21 );
		pShader->PI_SetModulationPixelShaderDynamicState_LinearScale_ScaleInW( PSREG_CONSTANT_21, flLScale );
		pShader->PI_SetModulationVertexShaderDynamicState_LinearScale( flLScale );
		pShader->PI_EndCommandBuffer();
	}
	DYNAMIC_STATE
	{
		pShaderAPI->SetDefaultState();

		// Bind textures
		pShader->BindTexture( SHADER_SAMPLER0, info.m_nBaseTexture, info.m_nBaseTextureFrame );							// Base Map 1
	
		pShaderAPI->BindStandardTexture( SHADER_SAMPLER5, TEXTURE_LIGHTMAP );

		if( bHasBump )
		{
			pShader->BindTexture( SHADER_SAMPLER3, info.m_nBumpmap, info.m_nBumpFrame );
		}

		if( bHasEnvmap )
		{
			pShader->BindTexture( SHADER_SAMPLER4, info.m_nEnvmap, info.m_nEnvmapFrame );
		}

		if ( bHasFoW )
		{
			pShader->BindTexture( SHADER_SAMPLER10, info.m_nFoW, -1 );

			float	vFoWSize[ 4 ];
			Vector	vMins = pShaderAPI->GetVectorRenderingParameter( VECTOR_RENDERPARM_GLOBAL_FOW_MINS );
			Vector	vMaxs = pShaderAPI->GetVectorRenderingParameter( VECTOR_RENDERPARM_GLOBAL_FOW_MAXS );
			vFoWSize[ 0 ] = vMins.x;
			vFoWSize[ 1 ] = vMins.y;
			vFoWSize[ 2 ] = vMaxs.x - vMins.x;
			vFoWSize[ 3 ] = vMaxs.y - vMins.y;
			pShaderAPI->SetVertexShaderConstant( 26, vFoWSize );
		}

		//Vector4D vLightDir;
		//vLightDir.AsVector3D() = pShaderAPI->GetVectorRenderingParameter( VECTOR_RENDERPARM_GLOBAL_LIGHT_DIRECTION );
		//vLightDir.w = pShaderAPI->GetFloatRenderingParameter( FLOAT_RENDERPARM_SPECULAR_POWER );
		//pShaderAPI->SetVertexShaderConstant( 29, vLightDir.Base() );


		LightState_t lightState;
		pShaderAPI->GetDX9LightState( &lightState );

		DECLARE_DYNAMIC_VERTEX_SHADER( lightmappedbrushgeneric_vs30 );
		//SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING,      pShaderAPI->GetCurrentNumBones() > 0 );
		SET_DYNAMIC_VERTEX_SHADER( lightmappedbrushgeneric_vs30 );

		DECLARE_DYNAMIC_PIXEL_SHADER( lightmappedbrushgeneric_ps30 );
		SET_DYNAMIC_PIXEL_SHADER( lightmappedbrushgeneric_ps30 );

		pShaderAPI->SetPixelShaderFogParams( PSREG_FOG_PARAMS );

		// Pack phong exponent in with the eye position
		float vEyePos_SpecExponent[4];
		float vSpecularTint[4] = {1, 1, 1, 1};
		pShaderAPI->GetWorldSpaceCameraPosition( vEyePos_SpecExponent );

//		if ( (info.m_nPhongExponent != -1) && params[info.m_nPhongExponent]->IsDefined() )
//			vEyePos_SpecExponent[3] = params[info.m_nPhongExponent]->GetFloatValue();		// This overrides the channel in the map
//		else
			vEyePos_SpecExponent[3] = 0;													// Use the alpha channel of the normal map for the exponent

		// If it's all zeros, there was no constant tint in the vmt
		if ( (vSpecularTint[0] == 0.0f) && (vSpecularTint[1] == 0.0f) && (vSpecularTint[2] == 0.0f) )
		{
			vSpecularTint[0] = 1.0f;
			vSpecularTint[1] = 1.0f;
			vSpecularTint[2] = 1.0f;
		}

		pShaderAPI->SetPixelShaderConstant( PSREG_EYEPOS_SPEC_EXPONENT, vEyePos_SpecExponent, 1 );

		if( bHasTeamColorTexture )
		{
			// Team color constant + sampler
			static const float kDefaultTeamColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			const float *vecTeamColor = (bHasTeamColorTexture && IS_PARAM_DEFINED( info.m_nTeamColor )) ? params[info.m_nTeamColor]->GetVecValue() : kDefaultTeamColor;
			pShaderAPI->SetPixelShaderConstant( PSREG_CONSTANT_19, vecTeamColor, 1 );
			pShader->BindTexture( SHADER_SAMPLER2, info.m_nTeamColorTexture, -1 );
		}

		// Set c0 and c1 to contain first two rows of ViewProj matrix
		/*VMatrix matView, matProj, matViewProj;
		pShaderAPI->GetMatrix( MATERIAL_VIEW, matView.m[0] );
		pShaderAPI->GetMatrix( MATERIAL_PROJECTION, matProj.m[0] );
		matViewProj = matView * matProj;
		pShaderAPI->SetPixelShaderConstant( 0, matViewProj.m[0], 2 );*/

		// Set c2 to envmap tint
		float envmapTintVal[4];
		params[info.m_nEnvmapTint]->GetVecValue( envmapTintVal, 3 );
		pShaderAPI->SetPixelShaderConstant( PSREG_CONSTANT_18, envmapTintVal );
	}
	pShader->Draw();
}
