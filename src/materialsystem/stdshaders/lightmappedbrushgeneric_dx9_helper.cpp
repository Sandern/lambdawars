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
#include "lightmappedbrushgeneric_vs20.inc"
#include "lightmappedbrushgeneric_vs30.inc"
#include "lightmappedbrushgeneric_ps20b.inc"
#include "lightmappedbrushgeneric_ps30.inc"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"


// FIXME: doesn't support fresnel!
void InitParamsLightmappedBrushGeneric_DX9( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, LightmappedBrushGeneric_DX9_Vars_t &info )
{
	SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );

	params[FLASHLIGHTTEXTURE]->SetStringValue( GetFlashlightTextureFilename() );

	SET_PARAM_STRING_IF_NOT_DEFINED( info.m_nFoW, "_rt_fog_of_war" );
}

void InitLightmappedBrushGeneric_DX9( CBaseVSShader *pShader, IMaterialVar** params, LightmappedBrushGeneric_DX9_Vars_t &info )
{
	if ( params[ info.m_nBaseTexture ]->IsDefined() )
	{
		pShader->LoadTexture( info.m_nBaseTexture );
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
	bool bDraw = true;

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

	SHADOW_STATE
	{
		pShader->SetInitialShadowState( );

		pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER1, true );	// Always SRGB read on base map 1

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

		if ( bHasFoW )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER10, true );
		}

		if( bHasTeamColorTexture )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
		}

		unsigned int flags = VERTEX_POSITION;
		int nTexCoordCount = 2;

		pShaderShadow->VertexShaderVertexFormat( flags, nTexCoordCount, 0, 0 );

		if ( !g_pHardwareConfig->HasFastVertexTextures() )
		{
			DECLARE_STATIC_VERTEX_SHADER( lightmappedbrushgeneric_vs20 );
			SET_STATIC_VERTEX_SHADER_COMBO( FOW, bHasFoW );
			SET_STATIC_VERTEX_SHADER( lightmappedbrushgeneric_vs20 );

			// Bind ps_2_b shader so we can get Phong terms
			if ( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_STATIC_PIXEL_SHADER( lightmappedbrushgeneric_ps20b );
				SET_STATIC_PIXEL_SHADER_COMBO( LIGHTING_PREVIEW, nLightingPreviewMode );
				SET_STATIC_PIXEL_SHADER_COMBO( FOW, bHasFoW );
				SET_STATIC_PIXEL_SHADER_COMBO( TEAMCOLORTEXTURE, bHasTeamColorTexture );
				SET_STATIC_PIXEL_SHADER( lightmappedbrushgeneric_ps20b );
			}
			else
			{
				bDraw = false;
			}
		}
		else
		{
			// The vertex shader uses the vertex id stream
			SET_FLAGS2( MATERIAL_VAR2_USES_VERTEXID );

			DECLARE_STATIC_VERTEX_SHADER( lightmappedbrushgeneric_vs30 );
			SET_STATIC_VERTEX_SHADER_COMBO( FOW, bHasFoW );
			SET_STATIC_VERTEX_SHADER( lightmappedbrushgeneric_vs30 );

			// Bind ps_2_b shader so we can get Phong terms
			DECLARE_STATIC_PIXEL_SHADER( lightmappedbrushgeneric_ps30 );
			SET_STATIC_PIXEL_SHADER_COMBO( LIGHTING_PREVIEW, nLightingPreviewMode );
			SET_STATIC_PIXEL_SHADER_COMBO( FOW, bHasFoW );
			SET_STATIC_PIXEL_SHADER_COMBO( TEAMCOLORTEXTURE, bHasTeamColorTexture );
			SET_STATIC_PIXEL_SHADER( lightmappedbrushgeneric_ps30 );
		}

		pShader->DefaultFog();

		float flLScale = pShaderShadow->GetLightMapScaleFactor();

		// Lighting constants
		pShader->PI_BeginCommandBuffer();
		pShader->PI_SetPixelShaderAmbientLightCube( PSREG_AMBIENT_CUBE );
//		pShader->PI_SetPixelShaderLocalLighting( PSREG_LIGHT_INFO_ARRAY );
		pShader->PI_SetModulationPixelShaderDynamicState_LinearScale_ScaleInW( PSREG_CONSTANT_21, flLScale );
		pShader->PI_EndCommandBuffer();
	}
	DYNAMIC_STATE
	{
		pShaderAPI->SetDefaultState();

		// Bind textures
		pShader->BindTexture( SHADER_SAMPLER1, info.m_nBaseTexture );							// Base Map 1
	
		pShaderAPI->BindStandardTexture( SHADER_SAMPLER5, TEXTURE_LIGHTMAP );


#if 0
		if( bHasFlashlight )
		{
			VMatrix worldToTexture;
			ITexture *pFlashlightDepthTexture;
			FlashlightState_t state = pShaderAPI->GetFlashlightStateEx( worldToTexture, &pFlashlightDepthTexture );
			
			pShader->BindTexture( SHADER_SAMPLER13, state.m_pSpotlightTexture, state.m_nSpotlightTextureFrame );

			//bFlashlightShadows = state.m_bEnableShadows;

			SetFlashLightColorFromState( state, pShaderAPI, PSREG_FLASHLIGHT_COLOR );

			if( pFlashlightDepthTexture && g_pConfig->ShadowDepthTexture() && state.m_bEnableShadows )
			{
				pShader->BindTexture( SHADER_SAMPLER14, pFlashlightDepthTexture );
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER15, TEXTURE_SHADOW_NOISE_2D );
			}

			float atten[4], pos[4], tweaks[4];

			atten[0] = state.m_fConstantAtten;		// Set the flashlight attenuation factors
			atten[1] = state.m_fLinearAtten;
			atten[2] = state.m_fQuadraticAtten;
			atten[3] = state.m_FarZAtten;
			pShaderAPI->SetPixelShaderConstant( PSREG_FLASHLIGHT_ATTENUATION, atten, 1 );

			pos[0] = state.m_vecLightOrigin[0];		// Set the flashlight origin
			pos[1] = state.m_vecLightOrigin[1];
			pos[2] = state.m_vecLightOrigin[2];
			pos[3] = state.m_FarZ;
			pShaderAPI->SetPixelShaderConstant( PSREG_FLASHLIGHT_POSITION_RIM_BOOST, pos, 1 );	// steps on rim boost

			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, worldToTexture.Base(), 4 );

			// Tweaks associated with a given flashlight
			tweaks[0] = ShadowFilterFromState( state );
			tweaks[1] = ShadowAttenFromState( state );
			pShader->HashShadow2DJitter( state.m_flShadowJitterSeed, &tweaks[2], &tweaks[3] );
			pShaderAPI->SetPixelShaderConstant( PSREG_ENVMAP_TINT__SHADOW_TWEAKS, tweaks, 1 );

			// Dimensions of screen, used for screen-space noise map sampling
			float vScreenScale[4] = {1280.0f / 32.0f, 720.0f / 32.0f, 0, 0};
			int nWidth, nHeight;
			pShaderAPI->GetBackBufferDimensions( nWidth, nHeight );

			int nTexWidth, nTexHeight;
			pShaderAPI->GetStandardTextureDimensions( &nTexWidth, &nTexHeight, TEXTURE_SHADOW_NOISE_2D );

			vScreenScale[0] = (float) nWidth  / nTexWidth;
			vScreenScale[1] = (float) nHeight / nTexHeight;

			pShaderAPI->SetPixelShaderConstant( PSREG_FLASHLIGHT_SCREEN_SCALE, vScreenScale, 1 );

			if ( IsX360() )
			{
				pShaderAPI->SetBooleanPixelShaderConstant( 0, &state.m_nShadowQuality, 1 );
			}

			QAngle angles;
			QuaternionAngles( state.m_quatOrientation, angles );

#if 0
			// World to Light's View matrix
			matrix3x4_t viewMatrix, viewMatrixInverse;
			AngleMatrix( angles, state.m_vecLightOrigin, viewMatrixInverse );
			MatrixInvert( viewMatrixInverse, viewMatrix );
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4, worldToTexture.Base(), 4 );
#endif
		}
#endif

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

		Vector4D vLightDir;
		vLightDir.AsVector3D() = pShaderAPI->GetVectorRenderingParameter( VECTOR_RENDERPARM_GLOBAL_LIGHT_DIRECTION );
		vLightDir.w = pShaderAPI->GetFloatRenderingParameter( FLOAT_RENDERPARM_SPECULAR_POWER );
		pShaderAPI->SetVertexShaderConstant( 29, vLightDir.Base() );


		LightState_t lightState;
		pShaderAPI->GetDX9LightState( &lightState );

		if ( !g_pHardwareConfig->HasFastVertexTextures() )
		{
			DECLARE_DYNAMIC_VERTEX_SHADER( lightmappedbrushgeneric_vs20 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING,      pShaderAPI->GetCurrentNumBones() > 0 );
			SET_DYNAMIC_VERTEX_SHADER( lightmappedbrushgeneric_vs20 );

			// Bind ps_2_b shader so we can get Phong, rim and a cloudier refraction
			if ( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( lightmappedbrushgeneric_ps20b );
				SET_DYNAMIC_PIXEL_SHADER( lightmappedbrushgeneric_ps20b );
			}
			else
			{
				bDraw = false;
			}
		}
		else
		{
			DECLARE_DYNAMIC_VERTEX_SHADER( lightmappedbrushgeneric_vs30 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING,      pShaderAPI->GetCurrentNumBones() > 0 );
			SET_DYNAMIC_VERTEX_SHADER( lightmappedbrushgeneric_vs30 );

			DECLARE_DYNAMIC_PIXEL_SHADER( lightmappedbrushgeneric_ps30 );
			SET_DYNAMIC_PIXEL_SHADER( lightmappedbrushgeneric_ps30 );
		}
		
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
			pShaderAPI->SetPixelShaderConstant( 19, vecTeamColor, 1 );
			pShader->BindTexture( SHADER_SAMPLER2, info.m_nTeamColorTexture, -1 );
		}

		// Set c0 and c1 to contain first two rows of ViewProj matrix
		VMatrix matView, matProj, matViewProj;
		pShaderAPI->GetMatrix( MATERIAL_VIEW, matView.m[0] );
		pShaderAPI->GetMatrix( MATERIAL_PROJECTION, matProj.m[0] );
		matViewProj = matView * matProj;
		pShaderAPI->SetPixelShaderConstant( 0, matViewProj.m[0], 2 );

		pShaderAPI->SetPixelShaderFogParams( PSREG_FOG_PARAMS );
	}
	pShader->Draw( bDraw );
}
