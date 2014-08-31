
#include "deferred_includes.h"

#include "webview_vs30.inc"
#include "webview_ps30.inc"

#include "tier0/memdbgon.h"


BEGIN_VS_SHADER(WebView, "")
BEGIN_SHADER_PARAMS
END_SHADER_PARAMS

SHADER_INIT_PARAMS()
{
	if (g_pHardwareConfig->HasFastVertexTextures())
		SET_FLAGS2(MATERIAL_VAR2_USES_VERTEXID);
}

SHADER_INIT
{
	if (params[BASETEXTURE]->IsDefined())
	{
		LoadTexture(BASETEXTURE);
	}
}

SHADER_FALLBACK
{
	return 0;
}

SHADER_DRAW
{
	SHADOW_STATE
	{
		SetInitialShadowState();

		DefaultFog();

		pShaderShadow->EnableTexture(SHADER_SAMPLER0, true);

		int iVFmtFlags = VERTEX_POSITION;
		int iUserDataSize = 0;

		// texcoord0 : base texcoord
		int pTexCoordDim[3] = { 2, 2, 3 };
		int nTexCoordCount = 1;

		// This shader supports compressed vertices, so OR in that flag:
		iVFmtFlags |= VERTEX_FORMAT_COMPRESSED;

		pShaderShadow->VertexShaderVertexFormat(iVFmtFlags, nTexCoordCount, pTexCoordDim, iUserDataSize);

		// The vertex shader uses the vertex id stream
		if (g_pHardwareConfig->HasFastVertexTextures())
		{
			SET_FLAGS2(MATERIAL_VAR2_USES_VERTEXID);
		}

		SetBlendingShadowState( EvaluateBlendRequirements(BASETEXTURE, true) );

		DECLARE_STATIC_VERTEX_SHADER(webview_vs30);
		SET_STATIC_VERTEX_SHADER(webview_vs30);

		DECLARE_STATIC_PIXEL_SHADER(webview_ps30);
		SET_STATIC_PIXEL_SHADER(webview_ps30);
	}
	DYNAMIC_STATE
	{
		pShaderAPI->SetDefaultState();

		BindTexture(SHADER_SAMPLER0, BASETEXTURE);							// Base Map 1

		DECLARE_DYNAMIC_VERTEX_SHADER(webview_vs30);
		SET_DYNAMIC_VERTEX_SHADER(webview_vs30);

		DECLARE_DYNAMIC_PIXEL_SHADER(webview_ps30);
		SET_DYNAMIC_PIXEL_SHADER(webview_ps30);
	}
	Draw();
}

END_SHADER
