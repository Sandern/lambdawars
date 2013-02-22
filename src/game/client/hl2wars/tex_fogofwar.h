//====== Copyright © 2013 Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef TEXFOW_H
#define TEXFOW_H

#ifdef _WIN32
#pragma once
#endif

#include "materialsystem/ITexture.h"
//#include "ProxyEntity.h"

enum {
	TEXFOW_NONE = 0,
	TEXFOW_DIRECT, // Directly copy the fow state
	TEXFOW_INDIRECT, // Copy the smoothed/converged data
};

extern int g_bFOWUpdateType;

class CFOWMaterialProxy;

class CFOWTextureRegen : public ITextureRegenerator
{
public:
	CFOWTextureRegen(){}
	//CFOWTextureRegen( CFOWMaterialProxy *pProxy ) : m_pProxy(pProxy) {} 
	virtual void RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pSubRect );
	virtual void Release() {}

private:
	//CFOWMaterialProxy *m_pProxy;
};

/*
class CFOWMaterialProxy : public IMaterialProxy
{
public:
	CFOWMaterialProxy();
	virtual ~CFOWMaterialProxy();
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind(void *pC_BaseEntity );
	virtual void Release( void ) { }
	virtual IMaterial *GetMaterial();

private:

	IMaterial *m_pMaterial;
	IMaterialVar *m_pFOWTextureVar;
	CFOWTextureRegen *m_pTextureRegen;
	ITexture		*m_pTexture;      // The texture

};
*/

#endif // TEXFOW_H