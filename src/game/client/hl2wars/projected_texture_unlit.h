//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PROJECTEDTEXTUREUNLIT_H
#define PROJECTEDTEXTUREUNLIT_H
#ifdef _WIN32
#pragma once
#endif

class ProjectedTexture
{
public:
	ProjectedTexture();
	ProjectedTexture( const char *pMaterial, Vector vMin = Vector(0, 0, 0), Vector vMax = Vector(0, 0, 0), 
		Vector vOrigin = Vector(), QAngle qAngle = QAngle(0,0,0),
		int r = 255, int g = 255, int b = 255, int a = 255 );
	ProjectedTexture( const char *pMaterial, const char *pTextureGroupName, Vector vMin = Vector(0, 0, 0),
		Vector vMax = Vector(0, 0, 0), Vector vOrigin = Vector(), QAngle qAngle = QAngle(0,0,0),
		int r = 255, int g = 255, int b = 255, int a = 255 );
	ProjectedTexture( CMaterialReference &ref, Vector vMin = Vector(0, 0, 0), Vector vMax = Vector(0, 0, 0),
		Vector vOrigin = Vector(), QAngle qAngle = QAngle(0,0,0), 
		int r = 255, int g = 255, int b = 255, int a = 255 );

	~ProjectedTexture();

	void Init( const char *pMaterial, const char *pTextureGroupName = 0, Vector vMin = Vector(), 
		Vector vMax = Vector(), Vector vOrigin = Vector(), QAngle qAngle = QAngle(0,0,0),
		int r = 255, int g = 255, int b = 255, int a = 255 );
	void Init( CMaterialReference &ref, Vector vMin = Vector(), Vector vMax = Vector(), 
		Vector vOrigin = Vector(), QAngle qAngle = QAngle(0,0,0),
		int r = 255, int g = 255, int b = 255, int a = 255 );
	void Shutdown();

	void Update( Vector vOrigin, QAngle qAngle = QAngle(0,0,0) );
	void UpdateShadow();

	void SetMinMax( Vector &vMin, Vector vMax );
	void SetMaterial( const char *pMaterial, const char *pTextureGroupName = 0 );
	void SetMaterial( CMaterialReference &ref );
	void SetProjectionDistance( float dist );

private:
#if 0 //def  HL2WARS_ASW_DLL
	void SharedInit( Vector vMin = Vector(), Vector vMax = Vector(), 
		Vector vOrigin = Vector(), QAngle qAngle = QAngle(0,0,0),
		int r = 255, int g = 255, int b = 255, int a = 255 );

private:
	CTextureReference m_SpotlightTexture;
	FlashlightState_t m_Info;
#else
	ProjectTextureUnlitInfo_t m_Info;
#endif // HL2WARS_ASW_DLL
	ClientShadowHandle_t m_shadowHandle;
};

#endif // PROJECTEDTEXTUREUNLIT_H