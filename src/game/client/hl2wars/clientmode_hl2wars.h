//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef HL2Wars_CLIENTMODE_H
#define HL2Wars_CLIENTMODE_H
#ifdef _WIN32
#pragma once
#endif

#include "clientmode_shared.h"
#include "hl2warsviewport.h"

class IGameUI;

class ClientModeHL2WarsNormal : public ClientModeShared 
{
DECLARE_CLASS( ClientModeHL2WarsNormal, ClientModeShared );

private:

// IClientMode overrides.
public:

					ClientModeHL2WarsNormal();
	virtual			~ClientModeHL2WarsNormal();

	virtual void	Init();
	virtual void	InitViewport();
	virtual void	Shutdown();

	virtual int		KeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

	virtual void	SetLoadingBackgroundDialog( boost::python::object panel );

	HL2WarsViewport *GetViewPort() { return (HL2WarsViewport *)m_pViewport; }

	int				GetDeathMessageStartHeight( void );

	virtual void	PostRenderVGui();

	virtual bool	CanRecordDemo( char *errorMsg, int length ) const;

	virtual void	DoPostScreenSpaceEffects( const CViewSetup *pSetup );

private:
	
	//	void	UpdateSpectatorMode( void );

private:
	IGameUI			*m_pGameUI;
	boost::python::object m_refLoadingDialog;
};


extern IClientMode *GetClientModeNormal();
extern ClientModeHL2WarsNormal* GetClientModeHL2WarsNormal();


#endif // HL2Wars_CLIENTMODE_H
