
#include "deferred_includes.h"

#include "globallitgeneric_vs30.inc"
#include "globallitgeneric_ps30.inc"

#include "tier0/memdbgon.h"


BEGIN_VS_SHADER( GlobalLitSimple, "" )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( FULLBRIGHT, SHADER_PARAM_TYPE_BOOL, "0", "Render full bright" )

		SHADER_PARAM( FOW, SHADER_PARAM_TYPE_TEXTURE, "_rt_fog_of_war", "FoW Render Target" )
	END_SHADER_PARAMS

	void SetupParmsGBuffer( defParms_gBuffer &p )
	{
		p.bModel = false;

		p.iAlbedo = BASETEXTURE;
	}

	void SetupParmsShadow( defParms_shadow &p )
	{
		p.bModel = false;
		p.iAlbedo = BASETEXTURE;
	}

	/*void SetupParmsGlobalLitGeneric( defParms_composite &p )
	{
		p.bModel = false;
		p.iAlbedo = BASETEXTURE;
	}*/

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

		//defParms_composite parms_composite;
		//SetupParmsComposite( parms_composite );
		//InitPassComposite( parms_composite, this, params );

		if( params[BASETEXTURE]->IsDefined() )
		{
			LoadTexture( BASETEXTURE );
		}
	}

	SHADER_FALLBACK
	{
		Msg("%s fullbright? %d\n", params[BASETEXTURE]->GetStringValue(), params[FULLBRIGHT]->GetIntValue() );
		if( params[FULLBRIGHT]->GetIntValue() == 0 )
		{
			return "VertexLitGeneric";
		}

		return 0;
	}

	inline void DrawGlobalLitGeneric( IMaterialVar **params, IShaderShadow* pShaderShadow,
		IShaderDynamicAPI* pShaderAPI, VertexCompressionType_t vertexCompression, CBasePerMaterialContextData *pContextDataPtr ) 
	{
		const bool bHasFoW = false;

		SHADOW_STATE
		{
			SetInitialShadowState( );

			DefaultFog();

			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );

			int iVFmtFlags = VERTEX_POSITION;
			int iUserDataSize = 0;

			// texcoord0 : base texcoord
			int pTexCoordDim[3] = { 2, 2, 3 };
			int nTexCoordCount = 1;

			// This shader supports compressed vertices, so OR in that flag:
			iVFmtFlags |= VERTEX_FORMAT_COMPRESSED;

			pShaderShadow->VertexShaderVertexFormat( iVFmtFlags, nTexCoordCount, pTexCoordDim, iUserDataSize );

			// The vertex shader uses the vertex id stream
			if( g_pHardwareConfig->HasFastVertexTextures() )
			{
				SET_FLAGS2( MATERIAL_VAR2_USES_VERTEXID );
				SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_TESSELLATION );
			}

			DECLARE_STATIC_VERTEX_SHADER( globallitgeneric_vs30 );
			SET_STATIC_VERTEX_SHADER_COMBO( FOW, bHasFoW );
			SET_STATIC_VERTEX_SHADER( globallitgeneric_vs30 );

			DECLARE_STATIC_PIXEL_SHADER( globallitgeneric_ps30 );
			SET_STATIC_PIXEL_SHADER_COMBO( FOW, bHasFoW );
			SET_STATIC_PIXEL_SHADER( globallitgeneric_ps30 );

		}
		DYNAMIC_STATE
		{
			pShaderAPI->SetDefaultState();

			BindTexture( SHADER_SAMPLER1, BASETEXTURE );							// Base Map 1

			DECLARE_DYNAMIC_VERTEX_SHADER( globallitgeneric_vs30 );
			SET_DYNAMIC_VERTEX_SHADER( globallitgeneric_vs30 );

			DECLARE_DYNAMIC_PIXEL_SHADER( globallitgeneric_ps30 );
			//SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
			SET_DYNAMIC_PIXEL_SHADER( globallitgeneric_ps30 );
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
			DrawGlobalLitGeneric( params, pShaderShadow, pShaderAPI, vertexCompression, pDefContext );
		}
		else
			Draw( false );

		if ( pShaderAPI != NULL && pDefContext->m_bMaterialVarsChanged )
			pDefContext->m_bMaterialVarsChanged = false;
	}

END_SHADER
