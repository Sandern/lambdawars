//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifdef CLIENT_DLL
	#define CEnvDetailController C_EnvDetailController
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Implementation of the class that controls detail prop fade distances
//-----------------------------------------------------------------------------
class CEnvDetailController : public CBaseEntity
{
public:
	DECLARE_CLASS( CEnvDetailController, CBaseEntity );
	DECLARE_NETWORKCLASS();

	CEnvDetailController();
	virtual ~CEnvDetailController();

#ifndef CLIENT_DLL
	virtual bool KeyValue( const char *szKeyName, const char *szValue );
#else
	virtual void OnDataChanged( DataUpdateType_t updateType );
#endif // !CLIENT_DLL

	CNetworkVar( float, m_flFadeStartDist );
	CNetworkVar( float, m_flFadeEndDist );

	// ALWAYS transmit to all clients.
	virtual int UpdateTransmitState( void );

private:
#ifdef CLIENT_DLL
	float m_fOldFadeStartDist;
	float m_fOldFadeEndDist;
#endif // CLIENT_DLL
};

CEnvDetailController * GetDetailController();
