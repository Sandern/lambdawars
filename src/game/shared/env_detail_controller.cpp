//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "env_detail_controller.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(env_detail_controller,	CEnvDetailController);

IMPLEMENT_NETWORKCLASS_ALIASED( EnvDetailController, DT_DetailController )

BEGIN_NETWORK_TABLE_NOBASE( CEnvDetailController, DT_DetailController )
	#ifdef CLIENT_DLL
		RecvPropFloat( RECVINFO( m_flFadeStartDist ) ),
		RecvPropFloat( RECVINFO( m_flFadeEndDist ) ),
	#else
		SendPropFloat( SENDINFO( m_flFadeStartDist ) ),
		SendPropFloat( SENDINFO( m_flFadeEndDist ) ),
	#endif
END_NETWORK_TABLE()

static CEnvDetailController *s_detailController = NULL;
CEnvDetailController * GetDetailController()
{
	return s_detailController;
}

CEnvDetailController::CEnvDetailController()
{
	s_detailController = this;
}

CEnvDetailController::~CEnvDetailController()
{
	if ( s_detailController == this )
	{
		s_detailController = NULL;
	}
}

//--------------------------------------------------------------------------------------------------------------
int CEnvDetailController::UpdateTransmitState()
{
#ifndef CLIENT_DLL
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
#else
	return 0;
#endif
}

#ifndef CLIENT_DLL

	bool CEnvDetailController::KeyValue( const char *szKeyName, const char *szValue )
	{
		if (FStrEq(szKeyName, "fademindist"))
		{
			m_flFadeStartDist = atof(szValue);
		}
		else if (FStrEq(szKeyName, "fademaxdist"))
		{
			m_flFadeEndDist = atof(szValue);
		}

		return true;
	}

#else
	extern ConVar cl_detaildist;
	extern ConVar cl_detailfade;


	void CEnvDetailController::OnDataChanged( DataUpdateType_t updateType )
	{
		BaseClass::OnDataChanged( updateType );

		if( m_fOldFadeStartDist != m_flFadeStartDist )
		{
			cl_detailfade.SetValue( m_flFadeStartDist );
			m_fOldFadeStartDist = m_flFadeStartDist;
		}

		if( m_fOldFadeStartDist != m_flFadeEndDist )
		{
			cl_detaildist.SetValue( m_flFadeEndDist );
			m_fOldFadeEndDist = m_flFadeEndDist;
		}
	}

#endif // !CLIENT_DLL
