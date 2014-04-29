//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#include "BaseVSShader.h"
#include "deferred_includes.h"

// Auto generated inc files
#include "flora_vs30.inc"
#include "flora_ps30.inc"

BEGIN_VS_SHADER( Flora, "Help for Flora" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( ALPHATESTREFERENCE, SHADER_PARAM_TYPE_FLOAT, "0.5", "" )	
		SHADER_PARAM( FOW, SHADER_PARAM_TYPE_TEXTURE, "_rt_fog_of_war", "FoW Render Target" )

		// Tree sway animation control
		SHADER_PARAM( TREESWAY, SHADER_PARAM_TYPE_INTEGER, "0", "" );
		SHADER_PARAM( TREESWAYHEIGHT, SHADER_PARAM_TYPE_FLOAT, "1000", "" );
		SHADER_PARAM( TREESWAYSTARTHEIGHT, SHADER_PARAM_TYPE_FLOAT, "0.2", "" );
		SHADER_PARAM( TREESWAYRADIUS, SHADER_PARAM_TYPE_FLOAT, "300", "" );
		SHADER_PARAM( TREESWAYSTARTRADIUS, SHADER_PARAM_TYPE_FLOAT, "0.1", "" );
		SHADER_PARAM( TREESWAYSPEED, SHADER_PARAM_TYPE_FLOAT, "1", "" );
		SHADER_PARAM( TREESWAYSPEEDHIGHWINDMULTIPLIER, SHADER_PARAM_TYPE_FLOAT, "2", "" );
		SHADER_PARAM( TREESWAYSTRENGTH, SHADER_PARAM_TYPE_FLOAT, "10", "" );
		SHADER_PARAM( TREESWAYSCRUMBLESPEED, SHADER_PARAM_TYPE_FLOAT, "0.1", "" );
		SHADER_PARAM( TREESWAYSCRUMBLESTRENGTH, SHADER_PARAM_TYPE_FLOAT, "0.1", "" );
		SHADER_PARAM( TREESWAYSCRUMBLEFREQUENCY, SHADER_PARAM_TYPE_FLOAT, "0.1", "" );
		SHADER_PARAM( TREESWAYFALLOFFEXP, SHADER_PARAM_TYPE_FLOAT, "1.5", "" );
		SHADER_PARAM( TREESWAYSCRUMBLEFALLOFFEXP, SHADER_PARAM_TYPE_FLOAT, "1.0", "" );
		SHADER_PARAM( TREESWAYSPEEDLERPSTART, SHADER_PARAM_TYPE_FLOAT, "3", "" );
		SHADER_PARAM( TREESWAYSPEEDLERPEND, SHADER_PARAM_TYPE_FLOAT, "6", "" );

		// Deferred control options
		SHADER_PARAM( NOSHADOWPASS, SHADER_PARAM_TYPE_BOOL, "0", "Allows turning off the shadow pass of this material" )
		SHADER_PARAM( NODEFERREDLIGHT, SHADER_PARAM_TYPE_BOOL, "0", "No deferred light input" )
		SHADER_PARAM( MODELGLOBALNORMAL, SHADER_PARAM_TYPE_BOOL, "0", "Use global light direction as normal for all model vertices" )
		SHADER_PARAM( PHONG_EXP, SHADER_PARAM_TYPE_FLOAT, "", "" )
	END_SHADER_PARAMS

	void SetupParmsGBuffer( defParms_gBuffer &p )
	{
		p.bModel = false;

		p.iAlbedo = BASETEXTURE;
		p.iAlphatestRef = ALPHATESTREFERENCE;
		p.iPhongExp = PHONG_EXP;
	}

	void SetupParmsShadow( defParms_shadow &p )
	{
		p.bModel = false;
		p.iAlbedo = BASETEXTURE;
		p.iAlphatestRef = ALPHATESTREFERENCE;
	}

	bool DrawToGBuffer( IMaterialVar **params )
	{
#if DEFCFG_DEFERRED_SHADING == 1
		return true;
#else
		const bool bIsDecal = IS_FLAG_SET( MATERIAL_VAR_DECAL );
		const bool bTranslucent = IS_FLAG_SET( MATERIAL_VAR_TRANSLUCENT );

		return !bTranslucent && !bIsDecal;
#endif
	}

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );

		if ( g_pHardwareConfig->HasFastVertexTextures() )
			SET_FLAGS2( MATERIAL_VAR2_USES_VERTEXID );

		//const bool bIsDecal = IS_FLAG_SET( MATERIAL_VAR_DECAL );
		const bool bDeferredActive = GetDeferredExt()->IsDeferredLightingEnabled();
		if( bDeferredActive )// && !bIsDecal )
		{
			const bool bTranslucent = IS_FLAG_SET( MATERIAL_VAR_TRANSLUCENT );
			const bool bAlphaTest = IS_FLAG_SET( MATERIAL_VAR_ALPHATEST );

			if( bTranslucent ) 
			{
				CLEAR_FLAGS( MATERIAL_VAR_TRANSLUCENT );
				SET_FLAGS( MATERIAL_VAR_ALPHATEST );

				params[ALPHATESTREFERENCE]->SetFloatValue( 0.5f );
			}
			else if( bAlphaTest )
			{
				if( params[ALPHATESTREFERENCE]->GetFloatValue() == 0.0f )
					params[ALPHATESTREFERENCE]->SetFloatValue( 0.5f );
			}
		}

		const bool bDrawToGBuffer = DrawToGBuffer( params );

		if ( bDrawToGBuffer )
		{
			defParms_gBuffer parms_gbuffer;
			SetupParmsGBuffer( parms_gbuffer );
			InitParmsGBuffer( parms_gbuffer, this, params );
		
			defParms_shadow parms_shadow;
			SetupParmsShadow( parms_shadow );
			InitParmsShadowPass( parms_shadow, this, params );
		}
	}

	SHADER_FALLBACK
	{
		return 0;
	}

	SHADER_INIT
	{
		const bool bDrawToGBuffer = DrawToGBuffer( params );

		if ( bDrawToGBuffer )
		{
			defParms_gBuffer parms_gbuffer;
			SetupParmsGBuffer( parms_gbuffer );
			InitPassGBuffer( parms_gbuffer, this, params );

			defParms_shadow parms_shadow;
			SetupParmsShadow( parms_shadow );
			InitPassShadowPass( parms_shadow, this, params );
		}

		if( params[BASETEXTURE]->IsDefined() )
		{
			LoadTexture( BASETEXTURE );
		}
	}

	inline void DrawFlora( IMaterialVar **params, IShaderShadow* pShaderShadow,
		IShaderDynamicAPI* pShaderAPI, VertexCompressionType_t vertexCompression, CBasePerMaterialContextData *pContextDataPtr ) 
	{
		const bool bIsAlphaTested = IS_FLAG_SET( MATERIAL_VAR_ALPHATEST ) != 0;
		const bool bHasVertexColor = IS_FLAG_SET( MATERIAL_VAR_VERTEXCOLOR );

		SHADOW_STATE
		{
			SetInitialShadowState();

			// Alpha test: FIXME: shouldn't this be handled in Shader_t::SetInitialShadowState
			pShaderShadow->EnableAlphaTest( bIsAlphaTested );
			if ( params[ALPHATESTREFERENCE]->GetFloatValue() > 0.0f )
			{
				pShaderShadow->AlphaFunc( SHADER_ALPHAFUNC_GEQUAL, params[ALPHATESTREFERENCE]->GetFloatValue() );
			}

			DefaultFog();

			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER10, true ); // FOW
			pShaderShadow->EnableTexture( SHADER_SAMPLER13, true ); // Deferred light 1
			pShaderShadow->EnableTexture( SHADER_SAMPLER14, true ); // Deferred light 2

			int iVFmtFlags = VERTEX_POSITION;
			int iUserDataSize = 0;

			// texcoord0 : base texcoord
			int pTexCoordDim[3] = { 2, 2, 3 };
			int nTexCoordCount = 1;

			// This shader supports compressed vertices, so OR in that flag:
			iVFmtFlags |= VERTEX_FORMAT_COMPRESSED;

			if ( bHasVertexColor )
			{
				iVFmtFlags |= VERTEX_COLOR;
			}

			pShaderShadow->VertexShaderVertexFormat( iVFmtFlags, nTexCoordCount, pTexCoordDim, iUserDataSize );

			// The vertex shader uses the vertex id stream
			if( g_pHardwareConfig->HasFastVertexTextures() )
			{
				SET_FLAGS2( MATERIAL_VAR2_USES_VERTEXID );
				SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_TESSELLATION );
			}

			// Vertex Shader
			DECLARE_STATIC_VERTEX_SHADER( flora_vs30 );
			SET_STATIC_VERTEX_SHADER_COMBO( VERTEXCOLOR, bHasVertexColor );
			SET_STATIC_VERTEX_SHADER( flora_vs30 );

			// Pixel Shader
			DECLARE_STATIC_PIXEL_SHADER( flora_ps30 );
			SET_STATIC_PIXEL_SHADER_COMBO( VERTEXCOLOR, bHasVertexColor );
			SET_STATIC_PIXEL_SHADER( flora_ps30 );

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
			// Reset render state
			pShaderAPI->SetDefaultState();

			BindTexture( SHADER_SAMPLER0, BASETEXTURE );							// Base Map 1

			//if ( bHasFoW )
			{
				BindTexture( SHADER_SAMPLER10, FOW, -1 );

				float	vFoWSize[ 4 ];
				Vector	vMins = pShaderAPI->GetVectorRenderingParameter( VECTOR_RENDERPARM_GLOBAL_FOW_MINS );
				Vector	vMaxs = pShaderAPI->GetVectorRenderingParameter( VECTOR_RENDERPARM_GLOBAL_FOW_MAXS );
				vFoWSize[ 0 ] = vMins.x;
				vFoWSize[ 1 ] = vMins.y;
				vFoWSize[ 2 ] = vMaxs.x - vMins.x;
				vFoWSize[ 3 ] = vMaxs.y - vMins.y;
				pShaderAPI->SetVertexShaderConstant( 26, vFoWSize );
			}

			BindTexture( SHADER_SAMPLER13, GetDeferredExt()->GetTexture_LightAccum()  );
			BindTexture( SHADER_SAMPLER14, GetDeferredExt()->GetTexture_LightAccum2()  );
			int x, y, w, t;
			pShaderAPI->GetCurrentViewport( x, y, w, t );
			float fl1[4] = { 1.0f / w, 1.0f / t, 0, 0 };

			pShaderAPI->SetPixelShaderConstant( 3, fl1 );

			// Set Vertex Shader Combos
			DECLARE_DYNAMIC_VERTEX_SHADER( flora_vs30 );
			SET_DYNAMIC_VERTEX_SHADER( flora_vs30 );

			// Set Pixel Shader Combos
			DECLARE_DYNAMIC_PIXEL_SHADER( flora_ps30 );
			SET_DYNAMIC_PIXEL_SHADER( flora_ps30 );
		}
		Draw();
	}

	SHADER_DRAW
	{
		if ( pShaderAPI != NULL && *pContextDataPtr == NULL )
			*pContextDataPtr = new CDeferredPerMaterialContextData();

		CDeferredPerMaterialContextData *pDefContext = reinterpret_cast< CDeferredPerMaterialContextData* >(*pContextDataPtr);

		const int iDeferredRenderStage = pShaderAPI ?
			pShaderAPI->GetIntRenderingParameter( INT_RENDERPARM_DEFERRED_RENDER_STAGE )
			: DEFERRED_RENDER_STAGE_INVALID;

		const bool bDrawToGBuffer = DrawToGBuffer( params );

		Assert( pShaderAPI == NULL ||
			iDeferredRenderStage != DEFERRED_RENDER_STAGE_INVALID );

		if ( bDrawToGBuffer )
		{
			if ( pShaderShadow != NULL ||
				iDeferredRenderStage == DEFERRED_RENDER_STAGE_GBUFFER )
			{
				defParms_gBuffer parms_gbuffer;
				SetupParmsGBuffer( parms_gbuffer );
				DrawPassGBuffer( parms_gbuffer, this, params, pShaderShadow, pShaderAPI,
					vertexCompression, pDefContext );
			}
			else
				Draw( false );

			if ( pShaderShadow != NULL ||
				iDeferredRenderStage == DEFERRED_RENDER_STAGE_SHADOWPASS )
			{
				defParms_shadow parms_shadow;
				SetupParmsShadow( parms_shadow );
				DrawPassShadowPass( parms_shadow, this, params, pShaderShadow, pShaderAPI,
					vertexCompression, pDefContext );
			}
			else
				Draw( false );
		}

		if ( pShaderShadow != NULL ||
			iDeferredRenderStage == DEFERRED_RENDER_STAGE_COMPOSITION )
		{
			DrawFlora( params, pShaderShadow, pShaderAPI, vertexCompression, pDefContext );
		}
		else
			Draw( false );

		if ( pShaderAPI != NULL && pDefContext->m_bMaterialVarsChanged )
			pDefContext->m_bMaterialVarsChanged = false;
	}
END_SHADER
