//====== Copyright © 2013 Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "teamcolor_proxy.h"
#include "materialsystem/IMaterialProxy.h"
#include "materialsystem/IMaterialVar.h"
#include "imaterialproxydict.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CCoreOriginProxy : public IMaterialProxy
{
public:
	void OnBind( void *pRenderable );

	bool Init( IMaterial* pMaterial, KeyValues *pKeyValues );
	void Release() { delete this; }
	IMaterial *	GetMaterial() { return m_pMaterial; }

private:
	IMaterial *m_pMaterial;
	IMaterialVar *m_pSphereOriginTextureVar;
};

bool CCoreOriginProxy::Init( IMaterial* pMaterial, KeyValues *pKeyValues )
{
	bool found;

	m_pMaterial = pMaterial; 
	m_pSphereOriginTextureVar = pMaterial->FindVar( "$sphereorigin", &found );
	return true;
}

void CCoreOriginProxy::OnBind( void *pRenderable )
{
	if( !pRenderable || !m_pSphereOriginTextureVar )
		return;

	IClientRenderable *pRend = ( IClientRenderable* )pRenderable;
	if ( !pRend )
		return;

	const Vector &origin = pRend->GetRenderOrigin();

	m_pSphereOriginTextureVar->SetVecValue(origin[0], origin[1], origin[2], 1);
}

EXPOSE_MATERIAL_PROXY( CCoreOriginProxy, CoreOrigin );