//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//

#ifndef FogOfWar_BLENDED_PASS_HELPER_H
#define FogOfWar_BLENDED_PASS_HELPER_H
#ifdef _WIN32
#pragma once
#endif

#include <string.h>

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CBaseVSShader;
class IMaterialVar;
class IShaderDynamicAPI;
class IShaderShadow;

//-----------------------------------------------------------------------------
// Init params/ init/ draw methods
//-----------------------------------------------------------------------------
struct FogOfWarBlendedPassVars_t
{
	FogOfWarBlendedPassVars_t() { memset( this, 0xFF, sizeof(FogOfWarBlendedPassVars_t) ); }

	int m_nBaseTexture;
	int m_nFogOfWarTexture;
};

// Default values (Arrays should only be vec[4])
static const float kDefaultFogOfWar[4] = { 0.0f, 0.0f, 0.0f, 0.0f }; // Disabled by default

void InitParamsFogOfWarBlendedPass( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, FogOfWarBlendedPassVars_t &info );
void InitFogOfWarBlendedPass( CBaseVSShader *pShader, IMaterialVar** params, FogOfWarBlendedPassVars_t &info );
void DrawFogOfWarBlendedPass( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
								  IShaderShadow* pShaderShadow, FogOfWarBlendedPassVars_t &info, VertexCompressionType_t vertexCompression );

#endif // FogOfWar_BLENDED_PASS_HELPER_H
