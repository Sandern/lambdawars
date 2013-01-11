//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef _INCLUDED_CLIENTMODE_WARS_H
#define _INCLUDED_CLIENTMODE_WARS_H
#ifdef _WIN32
#pragma once
#endif

#include "clientmode_shared.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui/Cursor.h>
#include "GameUI/igameui.h"

class CHudViewport;

class ClientModeSDK : public ClientModeShared
{
public:
	DECLARE_CLASS( ClientModeSDK, ClientModeShared );

	virtual void	Init();
	//virtual void	InitWeaponSelectionHudElement() { return; }
	virtual void	InitViewport();
	virtual void	Shutdown();

	virtual void	LevelInit( const char *newmap );
	virtual void	LevelShutdown( void );
	virtual void	FireGameEvent( IGameEvent *event );
	virtual void	DoPostScreenSpaceEffects( const CViewSetup *pSetup );
	virtual void SDK_CloseAllWindows();
	virtual void SDK_CloseAllWindowsFrom(vgui::Panel* pPanel);	

	virtual void	OnColorCorrectionWeightsReset( void );
	virtual float	GetColorCorrectionScale( void ) const { return 1.0f; }
	virtual void	ClearCurrentColorCorrection() { m_pCurrentColorCorrection = NULL; }

	virtual int		KeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

private:
	CHandle<C_ColorCorrection> m_pCurrentColorCorrection;
};

extern IClientMode *GetClientModeNormal();
extern vgui::HScheme g_hVGuiCombineScheme;

extern ClientModeSDK* GetClientModeSDK();

#endif // _INCLUDED_CLIENTMODE_WARS_H
