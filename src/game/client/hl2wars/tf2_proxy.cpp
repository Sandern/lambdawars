//====== Copyright © 2013 Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "materialsystem/IMaterialProxy.h"
#include "materialsystem/IMaterialVar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CBurnLevelProxy : public IMaterialProxy
{
public:
	void OnBind( void *pRenderable );

	bool Init( IMaterial* pMaterial, KeyValues *pKeyValues );
	void Release() { delete this; }
	IMaterial *	GetMaterial() { return m_pMaterial; }

private:
	IMaterial *m_pMaterial;
	IMaterial *m_pDetailMaterial;
	IMaterialVar *m_pDetailVar;
	IMaterialVar *m_pDetailBlendFactorVar;
};

bool CBurnLevelProxy::Init( IMaterial* pMaterial, KeyValues *pKeyValues )
{
	bool found;

	m_pMaterial=pMaterial; 
	m_pDetailBlendFactorVar = pMaterial->FindVar( "$detailblendfactor", &found );


	m_pDetailVar = pMaterial->FindVar( "$detail", &found );
	m_pDetailMaterial = (found ? materials->FindMaterial(m_pDetailVar->GetStringValue(), NULL) : NULL);
	return true;
}

void CBurnLevelProxy::OnBind( void *pRenderable )
{
	if( !pRenderable )
		return;

	IClientRenderable *pRend = ( IClientRenderable* )pRenderable;
	C_BaseEntity *pEnt = pRend->GetIClientUnknown()->GetBaseEntity();
	if ( !pEnt )
		return;

	if( m_pDetailVar )
	{
		//if( (pEnt->GetFlags() & FL_ONFIRE) )
			//m_pDetailMaterial->AlphaModulate(m_pDetailBlendFactorVar->GetFloatValue());
		//else
			//m_pDetailMaterial->AlphaModulate(1.0);
		//Msg("Alpha modulate: %f, flag? %d\n", m_pDetailMaterial->GetAlphaModulation(), (pEnt->GetFlags() & FL_ONFIRE));
		m_pDetailVar->SetStringValue(""); // Just clear for now, no idea how this should work.
	}
}

EXPOSE_INTERFACE( CBurnLevelProxy, IMaterialProxy, "BurnLevel" IMATERIAL_PROXY_INTERFACE_VERSION );

// Yellow Level
class CYellowLevelProxy : public IMaterialProxy
{
public:
	void OnBind( void *pRenderable );

	bool Init( IMaterial* pMaterial, KeyValues *pKeyValues );
	void Release() { delete this; }
	IMaterial *	GetMaterial() { return m_pMaterial; }

private:
	IMaterial *m_pMaterial;
	IMaterialVar *m_pYellowVar;
	IMaterialVar *m_pColor2Var;
};

bool CYellowLevelProxy::Init( IMaterial* pMaterial, KeyValues *pKeyValues )
{
	bool found;

	m_pMaterial=pMaterial; 
	m_pYellowVar = pMaterial->FindVar( "$yellow", &found );
	m_pColor2Var = pMaterial->FindVar( "$color2", &found );
	return true;
}

void CYellowLevelProxy::OnBind( void *pRenderable )
{
	if( !pRenderable )
		return;

	const float *vecv = m_pColor2Var->GetVecValue();
	m_pYellowVar->SetVecValue(vecv, 4);
}

EXPOSE_INTERFACE( CYellowLevelProxy, IMaterialProxy, "YellowLevel" IMATERIAL_PROXY_INTERFACE_VERSION );

// spy_invis
class CSpyInvisProxy : public IMaterialProxy
{
public:
	CSpyInvisProxy() {}
	virtual ~CSpyInvisProxy() {}
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues ) { return true; }
	virtual void OnBind( void *pC_BaseEntity ) {}
	virtual void Release( void ) { delete this; }
	virtual IMaterial *GetMaterial() { return NULL; }
};
EXPOSE_INTERFACE( CSpyInvisProxy, IMaterialProxy, "spy_invis" IMATERIAL_PROXY_INTERFACE_VERSION );

// InvulnLevel
class CInvulnLevelProxy : public IMaterialProxy
{
public:
	CInvulnLevelProxy() {}
	virtual ~CInvulnLevelProxy() {}
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues ) { return true; }
	virtual void OnBind( void *pC_BaseEntity ) {}
	virtual void Release( void ) { delete this; }
	virtual IMaterial *GetMaterial() { return NULL; }
};
EXPOSE_INTERFACE( CInvulnLevelProxy, IMaterialProxy, "InvulnLevel" IMATERIAL_PROXY_INTERFACE_VERSION );
