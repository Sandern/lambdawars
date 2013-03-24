//====== Copyright © 1996-2006, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef MULTIBLEND_DX9_HELPER_H
#define MULTIBLEND_DX9_HELPER_H
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
struct LightmappedBrushGeneric_DX9_Vars_t
{
	LightmappedBrushGeneric_DX9_Vars_t() { memset( this, 0xFF, sizeof( *this ) ); }

	int			m_nBaseTexture;
	int			m_nBaseTextureFrame;
	int			m_nBumpmap;
	int			m_nBumpFrame;
	int			m_nEnvmap;
	int			m_nEnvmapFrame;
	int			m_nEnvmapTint;

	int			m_nFoW;
	int			m_nTeamColor;
	int			m_nTeamColorTexture;
};

void InitParamsLightmappedBrushGeneric_DX9( CBaseVSShader *pShader, IMaterialVar** params, const char *pMaterialName, 
						   LightmappedBrushGeneric_DX9_Vars_t &info );
void InitLightmappedBrushGeneric_DX9( CBaseVSShader *pShader, IMaterialVar** params, LightmappedBrushGeneric_DX9_Vars_t &info );
void DrawLightmappedBrushGeneric_DX9( CBaseVSShader *pShader, IMaterialVar** params, IShaderDynamicAPI *pShaderAPI,
				   IShaderShadow* pShaderShadow, LightmappedBrushGeneric_DX9_Vars_t &info, VertexCompressionType_t vertexCompression,
				   CBasePerMaterialContextData **pContextDataPtr );

#endif // MULTIBLEND_DX9_HELPER_H
