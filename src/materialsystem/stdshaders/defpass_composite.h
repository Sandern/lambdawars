#ifndef DEFPASS_COMPOSITE_H
#define DEFPASS_COMPOSITE_H

class CDeferredPerMaterialContextData;

struct defParms_composite
{
	defParms_composite()
	{
		Q_memset( this, 0xFF, sizeof( defParms_composite ) );

		bModel = false;
	};

	// textures
	int iAlbedo;
	int iAlbedo2;
	int iAlbedo3;
	int iAlbedo4;
	int iEnvmap;
	int iEnvmapMask;
	int iEnvmapMask2;
#if 0
	int iBlendmodulate;
	int iBlendmodulate2;
	int iBlendmodulate3;
#endif // 0

	// Multiblend
	int iMultiblend;

	int nSpecTexture;
	int nSpecTexture2;
	int nSpecTexture3;
	int nSpecTexture4;

	int nRotation;
	int nRotation2;
	int nRotation3;
	int nRotation4;
	int nScale;
	int nScale2;
	int nScale3;
	int nScale4;

	// envmapping
	int iEnvmapTint;
	int iEnvmapSaturation;
	int iEnvmapContrast;
	int iEnvmapFresnel;

	// rimlight
	int iRimlightEnable;
	int iRimlightExponent;
	int iRimlightAlbedoScale;
	int iRimlightTint;
	int iRimlightModLight;

	// alpha
	int iAlphatestRef;

	// phong
	int iPhongScale;
	int iPhongFresnel;

	// self illum
	int iSelfIllumTint;
	int iSelfIllumMaskInEnvmapAlpha;
	int iSelfIllumFresnelModulate;
	int iSelfIllumMask;

#if 0
	// blendmod
	int iBlendmodulateTransform;
	int iBlendmodulateTransform2;
	int iBlendmodulateTransform3;
#endif // 0
	

	int iFresnelRanges;

	// config
	bool bModel;

	// Fog of war
	int m_nFoW;

	// Team color
	int m_nTeamColorTexture;
	int m_nTeamColor;
};


void InitParmsComposite( const defParms_composite &info, CBaseVSShader *pShader, IMaterialVar **params );
void InitPassComposite( const defParms_composite &info, CBaseVSShader *pShader, IMaterialVar **params );
void DrawPassComposite( const defParms_composite &info, CBaseVSShader *pShader, IMaterialVar **params,
	IShaderShadow* pShaderShadow, IShaderDynamicAPI* pShaderAPI,
	VertexCompressionType_t vertexCompression, CDeferredPerMaterialContextData *pDeferredContext );


#endif