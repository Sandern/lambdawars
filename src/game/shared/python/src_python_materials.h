//====== Copyright © 2013 Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SRC_PYTHON_MATERIALS_H
#define SRC_PYTHON_MATERIALS_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
	#include "bitmap/imageformat.h" //ImageFormat enum definition
#endif // CLIENT_DLL

#ifdef CLIENT_DLL
class CPyTextureRegen;

class PyProceduralTexture
{
public:
	friend class CPyTextureRegen;

	PyProceduralTexture( const char *pTextureName, const char *pTextureGroupName, int w, int h, ImageFormat fmt, int nFlags );
	~PyProceduralTexture();

	const char *GetName() { return m_sTextureName; }
	bool IsValid();
	void Shutdown();
	bool IsModified() { return m_bIsModified; }
	void Download();

	void SetAllPixels( int i );
	void SetPixel( int x, int y, int i );
	void SetPixel( int x, int y, int r, int g, int b, int a );
	void SetPixel( int x, int y, const Color &c );

	int GetDebugID() { return m_iDebugID; }

private:
	char m_sTextureName[256];
	int m_iWidth, m_iHeight, m_iChannels;
	CUtlVector< char > m_TextureData;
	CPyTextureRegen *m_pTextureRegen;
	CTextureReference m_RenderBuffer;
	bool m_bIsModified;
	int m_iDebugID;
};

void PyUpdateProceduralMaterials();
void PyShutdownProceduralMaterials();

#endif // CLIENT_DLL

#endif // SRC_PYTHON_MATERIALS_H