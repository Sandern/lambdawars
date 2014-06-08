//====== Copyright © Sandern Corporation, All rights reserved. ===========//
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

	ClientModeSDK();

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

	virtual void	Update( void );

private:
	void UpdatePostProcessingEffects();

private:
	const C_PostProcessController *m_pCurrentPostProcessController;
	PostProcessParameters_t m_CurrentPostProcessParameters;
	PostProcessParameters_t m_LerpStartPostProcessParameters, m_LerpEndPostProcessParameters;
	CountdownTimer m_PostProcessLerpTimer;

	CHandle<C_ColorCorrection> m_pCurrentColorCorrection;
};

extern IClientMode *GetClientModeNormal();
extern vgui::HScheme g_hVGuiCombineScheme;

extern ClientModeSDK* GetClientModeSDK();

#endif // _INCLUDED_CLIENTMODE_WARS_H
