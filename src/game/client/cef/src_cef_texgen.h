//====== Copyright © 2013 Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================

#ifndef SRC_CEF_TEXGEN_H
#define SRC_CEF_TEXGEN_H
#ifdef _WIN32
#pragma once
#endif

#include "materialsystem/ITexture.h"

class SrcCefBrowser;

//-----------------------------------------------------------------------------
// Purpose: Texture generation
//-----------------------------------------------------------------------------
class CCefTextureGenerator : public ITextureRegenerator
{
public:
	CCefTextureGenerator( SrcCefBrowser *pBrowser );
	virtual void RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pSubRect );
	virtual void Release() {}

	bool IsDirty() { return m_bIsDirty; }
	void MakeDirty() { m_bIsDirty = true; }

private:
	bool m_bIsDirty;
	SrcCefBrowser *m_pBrowser;
};

#endif // SRC_CEF_TEXGEN_H