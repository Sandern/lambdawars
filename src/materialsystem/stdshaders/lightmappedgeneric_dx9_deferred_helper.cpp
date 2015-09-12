//========= Copyright © 1996-2007, Valve LLC, All rights reserved. ============
//
// Purpose: Lightmap only shader
//
// $Header: $
// $NoKeywords: $
//=============================================================================

#include "lightmappedgeneric_dx9_helper.h"
#include "BaseVSShader.h"
#include "shaderlib/commandbuilder.h"
#include "convar.h"

#include "lightmappedgeneric_vs20.inc"
#include "lightmappedgeneric_ps20b.inc"

#include "lightmappedgeneric_deferred_vs30.inc"
#include "lightmappedgeneric_deferred_ps30.inc"

#include "tier0/vprof.h"

#include "../materialsystem_global.h"
#include "shaderapi/ishaderutil.h"

#include "tier0/memdbgon.h"

extern ConVar mat_disable_lightwarp;
extern ConVar mat_disable_fancy_blending;
extern ConVar mat_fullbright;

extern ConVar mat_ambient_light_r;
extern ConVar mat_ambient_light_g;
extern ConVar mat_ambient_light_b;

extern ConVar r_twopasspaint;

static CCommandBufferBuilder< CFixedCommandStorageBuffer< 512 > > tmpBuf;

void DrawLightmappedGeneric_Deferred_DX9( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow, 
								 LightmappedGeneric_DX9_Vars_t &info, CDeferredPerMaterialContextData *pDeferredContext )
{
	const bool bHasFoW = true;

	bool hasBaseTexture = params[info.m_nBaseTexture]->IsTexture();
	int nAlphaChannelTextureVar = hasBaseTexture ? (int)info.m_nBaseTexture : (int)info.m_nEnvmapMask;
	BlendType_t nBlendType = pShader->EvaluateBlendRequirements( nAlphaChannelTextureVar, hasBaseTexture );
	bool bIsAlphaTested = IS_FLAG_SET( MATERIAL_VAR_ALPHATEST ) != 0;
	bool bFullyOpaqueWithoutAlphaTest = (nBlendType != BT_BLENDADD) && (nBlendType != BT_BLEND); //&& (!hasFlashlight || IsX360()); //dest alpha is free for special use
	bool bFullyOpaque = bFullyOpaqueWithoutAlphaTest && !bIsAlphaTested;
	bool bHasOutline = IsBoolSet( info.m_nOutline, params );
	//bool bHasSoftEdges = IsBoolSet( info.m_nSoftEdges, params );
	bool hasEnvmapMask = params[info.m_nEnvmapMask]->IsTexture() && !bHasFoW;

	bool hasSelfIllum = IS_FLAG_SET( MATERIAL_VAR_SELFILLUM );
	bool hasBaseTexture2 = hasBaseTexture && params[info.m_nBaseTexture2]->IsTexture();
	bool hasDetailTexture = params[info.m_nDetail]->IsTexture();
	bool hasBump = ( params[info.m_nBumpmap]->IsTexture() ) && g_pConfig->UseBumpmapping();
	bool hasSSBump = hasBump && (info.m_nSelfShadowedBumpFlag != -1) &&	( params[info.m_nSelfShadowedBumpFlag]->GetIntValue() );
	bool hasBump2 = hasBump && params[info.m_nBumpmap2]->IsTexture();
	bool hasBumpMask = hasBump && hasBump2 && params[info.m_nBumpMask]->IsTexture() && !hasSelfIllum &&
		!hasDetailTexture && !hasBaseTexture2 && (params[info.m_nBaseTextureNoEnvmap]->GetIntValue() == 0);
	bool hasNormalMapAlphaEnvmapMask = g_pConfig->UseSpecular() && IS_FLAG_SET( MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK );
	bool hasLightWarpTexture = params[info.m_nLightWarpTexture]->IsTexture();
	bool bHasBlendModulateTexture = 
		(info.m_nBlendModulateTexture != -1) &&
		(params[info.m_nBlendModulateTexture]->IsTexture() );

	float envmapTintVal[4];
	float selfIllumTintVal[4];
	params[info.m_nEnvmapTint]->GetVecValue( envmapTintVal, 3 );
	params[info.m_nSelfIllumTint]->GetVecValue( selfIllumTintVal, 3 );
	float envmapContrast = params[info.m_nEnvmapContrast]->GetFloatValue();
	//float envmapSaturation = params[info.m_nEnvmapSaturation]->GetFloatValue();
	//float fresnelReflection = params[info.m_nFresnelReflection]->GetFloatValue();
	bool hasEnvmap = params[info.m_nEnvmap]->IsTexture();

	bool bSeamlessMapping = ( ( info.m_nSeamlessMappingScale != -1 ) && 
							( params[info.m_nSeamlessMappingScale]->GetFloatValue() != 0.0 ) );

	const bool bParallaxMapping = false; // ATM never supported

	SHADOW_STATE
	{
		// Set variables only needed in shadow state
		bool bShaderSrgbRead = ( IsX360() && IS_PARAM_DEFINED( info.m_nShaderSrgbRead360 ) && params[info.m_nShaderSrgbRead360]->GetIntValue() );
		bool hasVertexColor = IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR );
		bool hasDiffuseBumpmap = hasBump && (params[info.m_nNoDiffuseBumpLighting]->GetIntValue() == 0);

		int envmap_variant; //0 = no envmap, 1 = regular, 2 = darken in shadow mode
		if( hasEnvmap )
		{
			//only enabled darkened cubemap mode when the scale calls for it. And not supported in ps20 when also using a 2nd bumpmap
			envmap_variant = ((GetFloatParam( info.m_nEnvMapLightScale, params ) > 0.0f) && (g_pHardwareConfig->SupportsPixelShaders_2_b() || !hasBump2)) ? 2 : 1;
		}
		else
		{
			envmap_variant = 0; 
		}

		// Setup shadow state
		pShaderShadow->SetDefaultState();

		// base texture
		pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
		pShaderShadow->EnableSRGBRead( SHADER_SAMPLER0, !bShaderSrgbRead );

		// Lightmap texture
		pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
		if( g_pHardwareConfig->GetHDRType() == HDR_TYPE_NONE )
		{
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER1, true );
		}
		else
		{
			pShaderShadow->EnableSRGBRead( SHADER_SAMPLER1, false );
		}

		if( hasBump || hasNormalMapAlphaEnvmapMask )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER4, true );
		}
		if( hasBump2 )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER5, true );
		}
		if( hasBumpMask )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER8, true );
		}
		if( hasEnvmapMask )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER5, true );
		}

		// Fog of war sampler
		if( bHasFoW )
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER13, true );
		}

		// Deferred light accullumation sampler
		pShaderShadow->EnableTexture( SHADER_SAMPLER10, true );

		unsigned int flags = VERTEX_POSITION;

		if( hasVertexColor || hasBaseTexture2 || hasBump2 )
		{
			flags |= VERTEX_COLOR;
		}

		// texcoord0 : base texcoord
		// texcoord1 : lightmap texcoord
		// texcoord2 : lightmap texcoord offset
		int numTexCoords = 2;
		if( hasBump )
		{
			numTexCoords = 3;
		}
		
		int nLightingPreviewMode = !bHasFoW ? IS_FLAG2_SET( MATERIAL_VAR2_USE_GBUFFER0 ) + 2 * IS_FLAG2_SET( MATERIAL_VAR2_USE_GBUFFER1 ) : 0;

		pShaderShadow->VertexShaderVertexFormat( flags, numTexCoords, 0, 0 );

		// Pre-cache pixel shaders
		bool hasBaseAlphaEnvmapMask = IS_FLAG_SET( MATERIAL_VAR_BASEALPHAENVMAPMASK );

		int bumpmap_variant=(hasSSBump) ? 2 : hasBump;
		bool bMaskedBlending=( (info.m_nMaskedBlending != -1) &&
								(params[info.m_nMaskedBlending]->GetIntValue() != 0) );

		DECLARE_STATIC_VERTEX_SHADER( lightmappedgeneric_deferred_vs30 );
		SET_STATIC_VERTEX_SHADER_COMBO( ENVMAP_MASK,  hasEnvmapMask );
		SET_STATIC_VERTEX_SHADER_COMBO( TANGENTSPACE,  params[info.m_nEnvmap]->IsTexture() );
		SET_STATIC_VERTEX_SHADER_COMBO( BUMPMAP,  hasBump );
		SET_STATIC_VERTEX_SHADER_COMBO( DIFFUSEBUMPMAP, hasDiffuseBumpmap );
		SET_STATIC_VERTEX_SHADER_COMBO( VERTEXCOLOR, IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR ) );
		SET_STATIC_VERTEX_SHADER_COMBO( VERTEXALPHATEXBLENDFACTOR, hasBaseTexture2 || hasBump2 );
		SET_STATIC_VERTEX_SHADER_COMBO( BUMPMASK, hasBumpMask );
		SET_STATIC_VERTEX_SHADER_COMBO( LIGHTING_PREVIEW, nLightingPreviewMode );
		SET_STATIC_VERTEX_SHADER_COMBO( PARALLAX_MAPPING, bParallaxMapping );
		SET_STATIC_VERTEX_SHADER_COMBO( SEAMLESS, bSeamlessMapping );
		SET_STATIC_VERTEX_SHADER_COMBO( DETAILTEXTURE, hasDetailTexture );
		SET_STATIC_VERTEX_SHADER_COMBO( FANCY_BLENDING, bHasBlendModulateTexture );
		SET_STATIC_VERTEX_SHADER_COMBO( SELFILLUM,  hasSelfIllum );
		SET_STATIC_VERTEX_SHADER_COMBO( FOW, bHasFoW );
		SET_STATIC_VERTEX_SHADER( lightmappedgeneric_deferred_vs30 );

		DECLARE_STATIC_PIXEL_SHADER( lightmappedgeneric_deferred_ps30 );
		SET_STATIC_PIXEL_SHADER_COMBO( BASETEXTURE2, hasBaseTexture2 );
		SET_STATIC_PIXEL_SHADER_COMBO( BUMPMAP,  bumpmap_variant );
		SET_STATIC_PIXEL_SHADER_COMBO( BUMPMAP2, hasBump2 );
		//SET_STATIC_PIXEL_SHADER_COMBO( BUMPMASK, hasBumpMask );
		SET_STATIC_PIXEL_SHADER_COMBO( DIFFUSEBUMPMAP,  hasDiffuseBumpmap );
		//SET_STATIC_PIXEL_SHADER_COMBO( CUBEMAP,  envmap_variant );
		//SET_STATIC_PIXEL_SHADER_COMBO( ENVMAPMASK,  hasEnvmapMask );
		SET_STATIC_PIXEL_SHADER_COMBO( BASEALPHAENVMAPMASK,  hasBaseAlphaEnvmapMask );
		SET_STATIC_PIXEL_SHADER_COMBO( SELFILLUM,  hasSelfIllum );
		SET_STATIC_PIXEL_SHADER_COMBO( NORMALMAPALPHAENVMAPMASK,  hasNormalMapAlphaEnvmapMask );
		/*SET_STATIC_PIXEL_SHADER_COMBO( BASETEXTURENOENVMAP,  params[info.m_nBaseTextureNoEnvmap]->GetIntValue() );
		SET_STATIC_PIXEL_SHADER_COMBO( BASETEXTURE2NOENVMAP, params[info.m_nBaseTexture2NoEnvmap]->GetIntValue() );
		SET_STATIC_PIXEL_SHADER_COMBO( WARPLIGHTING, hasLightWarpTexture );
		SET_STATIC_PIXEL_SHADER_COMBO( FANCY_BLENDING, bHasBlendModulateTexture );*/
		SET_STATIC_PIXEL_SHADER_COMBO( MASKEDBLENDING, bMaskedBlending);
		/*SET_STATIC_PIXEL_SHADER_COMBO( SEAMLESS, bSeamlessMapping );
		SET_STATIC_PIXEL_SHADER_COMBO( OUTLINE, bHasOutline );
		SET_STATIC_PIXEL_SHADER_COMBO( SOFTEDGES, bHasSoftEdges );
		SET_STATIC_PIXEL_SHADER_COMBO( DETAIL_BLEND_MODE, nDetailBlendMode );*/
		SET_STATIC_PIXEL_SHADER_COMBO( PARALLAX_MAPPING, bParallaxMapping );
		SET_STATIC_PIXEL_SHADER_COMBO( SHADER_SRGB_READ, bShaderSrgbRead );
		SET_STATIC_PIXEL_SHADER_COMBO( LIGHTING_PREVIEW, nLightingPreviewMode );
		SET_STATIC_PIXEL_SHADER( lightmappedgeneric_deferred_ps30 );

		// HACK HACK HACK - enable alpha writes all the time so that we have them for
		// underwater stuff and writing depth to dest alpha
		// But only do it if we're not using the alpha already for translucency
		pShaderShadow->EnableAlphaWrites( bFullyOpaque );

		pShaderShadow->EnableSRGBWrite( true );

		pShader->DefaultFog();

		// NOTE: This isn't optimal. If $color2 is ever changed by a material
		// proxy, this code won't get re-run, but too bad. No time to make this work
		// Also note that if the lightmap scale factor changes
		// all shadow state blocks will be re-run, so that's ok
		float flLScale = pShaderShadow->GetLightMapScaleFactor();
		pShader->PI_BeginCommandBuffer();
		pShader->PI_SetModulationPixelShaderDynamicState( 21 );

		// MAINTOL4DMERGEFIXME
		// Need to reflect this change which is from this rel changelist since this constant set was moved from the dynamic block to here:
		// Change 578692 by Alex@alexv_rel on 2008/06/04 18:07:31
		//
		// Fix for portalareawindows in ep2 being rendered black. The color variable was being multipurposed for both the vs and ps differently where the ps doesn't care about alpha, but the vs does. Only applying the alpha2 DoD hack to the pixel shader constant where the alpha was never used in the first place and leaving alpha as is for the vs.

  		// color[3] *= ( IS_PARAM_DEFINED( info.m_nAlpha2 ) && params[ info.m_nAlpha2 ]->GetFloatValue() > 0.0f ) ? params[ info.m_nAlpha2 ]->GetFloatValue() : 1.0f;
  	  	// pContextData->m_SemiStaticCmdsOut.SetPixelShaderConstant( 12, color );

		pShader->PI_SetModulationPixelShaderDynamicState_LinearScale_ScaleInW( 12, flLScale );
		pShader->PI_SetModulationVertexShaderDynamicState_LinearScale( flLScale );
		pShader->PI_EndCommandBuffer();
	}
	DYNAMIC_STATE
	{
		Assert( pDeferredContext != NULL );

		if ( pDeferredContext->m_bMaterialVarsChanged || !pDeferredContext->HasCommands( CDeferredPerMaterialContextData::DEFSTAGE_COMPOSITE ) )
		{
			tmpBuf.Reset();

			MaterialFogMode_t fogType = pShaderAPI->GetSceneFogMode();

			bool bWriteDepthToAlpha;
			bool bWriteWaterFogToAlpha;
			if(  bFullyOpaque ) 
			{
				bWriteDepthToAlpha = pShaderAPI->ShouldWriteDepthToDestAlpha();
				bWriteWaterFogToAlpha = (fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z);
				AssertMsg( !(bWriteDepthToAlpha && bWriteWaterFogToAlpha), "Can't write two values to alpha at the same time." );
			}
			else
			{
				//can't write a special value to dest alpha if we're actually using as-intended alpha
				bWriteDepthToAlpha = false;
				bWriteWaterFogToAlpha = false;
			}

			bool bHasBlendMaskTransform= (
				(info.m_nBlendMaskTransform != -1) &&
				(info.m_nMaskedBlending != -1) &&
				(params[info.m_nMaskedBlending]->GetIntValue() ) &&
				( ! (params[info.m_nBumpTransform]->MatrixIsIdentity() ) ) );
			
			// If we don't have a texture transform, we don't have
			// to set vertex shader constants or run vertex shader instructions
			// for the texture transform.
			bool bHasTextureTransform = 
				!( params[info.m_nBaseTextureTransform]->MatrixIsIdentity() &&
				   params[info.m_nBumpTransform]->MatrixIsIdentity() &&
				   params[info.m_nBumpTransform2]->MatrixIsIdentity() &&
				   params[info.m_nEnvmapMaskTransform]->MatrixIsIdentity() );
			
			bHasTextureTransform |= bHasBlendMaskTransform;

			bool bVertexShaderFastPath = !bHasTextureTransform && !params[info.m_nDetail]->IsTexture();

			int nFixedLightingMode = pShaderAPI->GetIntRenderingParameter( INT_RENDERPARM_ENABLE_FIXED_LIGHTING );
			if( nFixedLightingMode != ENABLE_FIXED_LIGHTING_NONE )
			{
				if ( bHasOutline )
				{
					nFixedLightingMode = ENABLE_FIXED_LIGHTING_NONE;
				}
				else
				{
					bVertexShaderFastPath = false;
				}
			}

			// texture binds
			if( hasBaseTexture )
			{
				tmpBuf.BindTexture( pShader, SHADER_SAMPLER0, info.m_nBaseTexture, info.m_nBaseTextureFrame );
			}
			// handle mat_fullbright 2
			bool bLightingOnly = mat_fullbright.GetInt() == 2 && !IS_FLAG_SET( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
			if( bLightingOnly )
			{
				// BASE TEXTURE
				if( hasSelfIllum )
				{
					tmpBuf.BindStandardTexture( SHADER_SAMPLER0, TEXTURE_GREY_ALPHA_ZERO );
				}
				else
				{
					tmpBuf.BindStandardTexture( SHADER_SAMPLER0, TEXTURE_GREY );
				}

				// BASE TEXTURE 2	
				if( hasBaseTexture2 )
				{
					tmpBuf.BindStandardTexture( SHADER_SAMPLER7, TEXTURE_GREY );
				}

				// DETAIL TEXTURE
				if( hasDetailTexture )
				{
					tmpBuf.BindStandardTexture( SHADER_SAMPLER12, TEXTURE_GREY );
				}

				// disable color modulation
				float color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
				tmpBuf.SetVertexShaderConstant( VERTEX_SHADER_MODULATION_COLOR, color );

				// turn off environment mapping
				envmapTintVal[0] = 0.0f;
				envmapTintVal[1] = 0.0f;
				envmapTintVal[2] = 0.0f;
			}

			// always set the transform for detail textures since I'm assuming that you'll
			// always have a detailscale.
			if( hasDetailTexture )
			{
				tmpBuf.SetVertexShaderTextureScaledTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, info.m_nBaseTextureTransform, info.m_nDetailScale );
			}

			if( hasBaseTexture2 )
			{
				tmpBuf.BindTexture( pShader, SHADER_SAMPLER7, info.m_nBaseTexture2, info.m_nBaseTexture2Frame );
			}
			if( hasDetailTexture )
			{
				tmpBuf.BindTexture( pShader, SHADER_SAMPLER12, info.m_nDetail, info.m_nDetailFrame );
			}

			if( hasBump || hasNormalMapAlphaEnvmapMask )
			{
				if( !g_pConfig->m_bFastNoBump )
				{
					tmpBuf.BindTexture( pShader, SHADER_SAMPLER4, info.m_nBumpmap, info.m_nBumpFrame );
				}
				else
				{
					if( hasSSBump )
					{
						tmpBuf.BindStandardTexture( SHADER_SAMPLER4, TEXTURE_SSBUMP_FLAT );
					}
					else
					{
						tmpBuf.BindStandardTexture( SHADER_SAMPLER4, TEXTURE_NORMALMAP_FLAT );
					}
				}
			}
			if( hasBump2 )
			{
				if( !g_pConfig->m_bFastNoBump )
				{
					tmpBuf.BindTexture( pShader, SHADER_SAMPLER5, info.m_nBumpmap2, info.m_nBumpFrame2 );
				}
				else
				{
					if( hasSSBump )
					{
						tmpBuf.BindStandardTexture( SHADER_SAMPLER5, TEXTURE_NORMALMAP_FLAT );
					}
					else
					{
						tmpBuf.BindStandardTexture( SHADER_SAMPLER5, TEXTURE_SSBUMP_FLAT );
					}
				}
			}
			if( hasBumpMask )
			{
				if( !g_pConfig->m_bFastNoBump )
				{
					tmpBuf.BindTexture( pShader, SHADER_SAMPLER8, info.m_nBumpMask, -1 );
				}
				else
				{
					// this doesn't make sense
					tmpBuf.BindStandardTexture( SHADER_SAMPLER8, TEXTURE_NORMALMAP_FLAT );
				}
			}

			if( hasEnvmapMask )
			{
				tmpBuf.BindTexture( pShader, SHADER_SAMPLER5, info.m_nEnvmapMask, info.m_nEnvmapMaskFrame );
			}

			if ( hasLightWarpTexture )
			{
				tmpBuf.BindTexture( pShader, SHADER_SAMPLER6, info.m_nLightWarpTexture, -1 );
			}

			if ( bHasBlendModulateTexture )
			{
				tmpBuf.BindTexture( pShader, SHADER_SAMPLER3, info.m_nBlendModulateTexture, -1 );
			}

			if( bHasFoW )
			{
				if( ( info.m_nFoW != -1 ) && ( params[ info.m_nFoW ]->IsTexture() != 0 ) )
					tmpBuf.BindTexture( pShader, SHADER_SAMPLER13, info.m_nFoW, -1 );
				else
					tmpBuf.BindStandardTexture( SHADER_SAMPLER13, TEXTURE_WHITE );

				float	vFoWSize[ 4 ];
				Vector	vMins = pShaderAPI->GetVectorRenderingParameter( VECTOR_RENDERPARM_GLOBAL_FOW_MINS );
				Vector	vMaxs = pShaderAPI->GetVectorRenderingParameter( VECTOR_RENDERPARM_GLOBAL_FOW_MAXS );
				vFoWSize[ 0 ] = vMins.x;
				vFoWSize[ 1 ] = vMins.y;
				vFoWSize[ 2 ] = vMaxs.x - vMins.x;
				vFoWSize[ 3 ] = vMaxs.y - vMins.y;
				tmpBuf.SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_12, vFoWSize );
			}

			tmpBuf.BindTexture( pShader, SHADER_SAMPLER10, GetDeferredExt()->GetTexture_LightAccum(), -1 );
			int x, y, w, t;
			pShaderAPI->GetCurrentViewport( x, y, w, t );
			float fl1[4] = { 1.0f / w, 1.0f / t, 0, 0 };

			tmpBuf.SetPixelShaderConstant( PSREG_UBERLIGHT_SMOOTH_EDGE_0, fl1 );

			DECLARE_DYNAMIC_VERTEX_SHADER( lightmappedgeneric_deferred_vs30 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( FASTPATH,  bVertexShaderFastPath );
			SET_DYNAMIC_VERTEX_SHADER_CMD( tmpBuf, lightmappedgeneric_deferred_vs30 );

			bool bPixelShaderFastPath = true;
			DECLARE_DYNAMIC_PIXEL_SHADER( lightmappedgeneric_deferred_ps30);
			SET_DYNAMIC_PIXEL_SHADER_COMBO( FASTPATH,  bPixelShaderFastPath || bHasOutline );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( FASTPATHENVMAPCONTRAST,  bPixelShaderFastPath && envmapContrast == 1.0f );
			// Don't write fog to alpha if we're using translucency
			//SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITE_DEPTH_TO_DESTALPHA, bWriteDepthToAlpha );
			SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITEWATERFOGTODESTALPHA, bWriteWaterFogToAlpha );
			//SET_DYNAMIC_PIXEL_SHADER_COMBO( FLASHLIGHTSHADOWS, bFlashlightShadows );
			SET_DYNAMIC_PIXEL_SHADER_CMD( tmpBuf, lightmappedgeneric_deferred_ps30 );

			tmpBuf.End();

			pDeferredContext->SetCommands( CDeferredPerMaterialContextData::DEFSTAGE_COMPOSITE, tmpBuf.Copy() );
		}
		pShaderAPI->SetDefaultState();

		pShaderAPI->ExecuteCommandBuffer( pDeferredContext->GetCommands( CDeferredPerMaterialContextData::DEFSTAGE_COMPOSITE ) );
	}

	pShader->Draw();

#if 0
	if( IsPC() && (IS_FLAG_SET( MATERIAL_VAR_ALPHATEST ) != 0) && bFullyOpaqueWithoutAlphaTest )
	{
		//Alpha testing makes it so we can't write to dest alpha
		//Writing to depth makes it so later polygons can't write to dest alpha either
		//This leads to situations with garbage in dest alpha.

		//Fix it now by converting depth to dest alpha for any pixels that just wrote.
		pShader->DrawEqualDepthToDestAlpha();
	}
#endif // 0
}
