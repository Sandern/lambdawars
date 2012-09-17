//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "teamcolor_proxy.h"
#include "materialsystem/IMaterialProxy.h"
#include "materialsystem/IMaterialVar.h"

#ifdef HL2WARS_ASW_DLL
	#include "imaterialproxydict.h"
#endif // HL2WARS_ASW_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CTeamColorProxy : public IMaterialProxy
{
public:
	void OnBind( void *pRenderable );

	bool Init( IMaterial* pMaterial, KeyValues *pKeyValues );
	void Release() { delete this; }
	IMaterial *	GetMaterial() { return m_pMaterial; }

private:
	IMaterial *m_pMaterial;
	IMaterialVar *m_pColorTextureVar;
};

bool CTeamColorProxy::Init( IMaterial* pMaterial, KeyValues *pKeyValues )
{
	bool found;

	m_pMaterial = pMaterial; 
	m_pColorTextureVar = pMaterial->FindVar( "$teamcolor", &found );
	return true;
}

void CTeamColorProxy::OnBind( void *pRenderable )
{
	if( !pRenderable || !m_pColorTextureVar )
		return;

	IClientRenderable *pRend = ( IClientRenderable* )pRenderable;
	C_BaseEntity *pEnt = pRend->GetIClientUnknown()->GetBaseEntity();
	if ( !pEnt )
		return;

	Vector &teamcolor = pEnt->GetTeamColor();

	m_pColorTextureVar->SetVecValue(teamcolor[0], teamcolor[1], teamcolor[2], 1);
}

EXPOSE_MATERIAL_PROXY( CTeamColorProxy, TeamColor );