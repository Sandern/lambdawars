//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef TEXDENSITY_H
#define TEXDENSITY_H

#ifdef _WIN32
#pragma once
#endif

#include "materialsystem/ITexture.h"

class CDensityMaterialProxy;

class CDensityTextureRegen : public ITextureRegenerator
{
public:
	CDensityTextureRegen(){}
	CDensityTextureRegen( CDensityMaterialProxy *pProxy ) : m_pProxy(pProxy) {} 
	virtual void RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pSubRect );
	virtual void Release() {}

private:
	CDensityMaterialProxy *m_pProxy;
};

class CDensityMaterialProxy : public IMaterialProxy
{
public:
	CDensityMaterialProxy();
	virtual ~CDensityMaterialProxy();
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind(void *pC_BaseEntity );
	virtual void Release( void ) { }
	virtual IMaterial *GetMaterial();

private:

	IMaterial *m_pMaterial;
	IMaterialVar *m_pFOWTextureVar;
	CDensityTextureRegen *m_pTextureRegen;
	ITexture		*m_pTexture;      // The texture

};


#endif // TEXFOW_H