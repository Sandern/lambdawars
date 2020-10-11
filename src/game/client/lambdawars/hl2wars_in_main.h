//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================

#ifndef WARS_IN_MAIN_H
#define WARS_IN_MAIN_H
#ifdef _WIN32
#pragma once
#endif

#include "input.h"
#include "c_hl2wars_player.h"

#ifdef WIN32
	#include <winlite.h>
#endif // WIN32

//#define USECUSTOMMOUSECLIPPING

//-----------------------------------------------------------------------------
// Purpose: HL2Wars Input interface
//-----------------------------------------------------------------------------
class CHL2WarsInput : public CInput
{
public:
							CHL2WarsInput( void );

	virtual		void		Init_All( void );
	virtual		void		Shutdown_All( void );

	virtual		void		LevelInit( void );

	virtual		void		CreateMove ( int sequence_number, float input_sample_frametime, bool active );
	virtual		void		ExtraMouseSample( float frametime, bool active );

	virtual		int			GetButtonBits( bool bResetState );

	virtual		void		AdjustAngles ( int nSlot, float frametime );

	virtual		void		ClampAngles( QAngle& viewangles );

	virtual		void		ResetMouse( void );

	virtual		void		GetFullscreenMousePos( int *mx, int *my, int *unclampedx = NULL, int *unclampedy = NULL );

	virtual void			UpdateMouseAim( C_HL2WarsPlayer *pPlayer, int x, int y );
	virtual void			CalculateMouseAim( C_HL2WarsPlayer *pPlayer, int x, int y, Vector &result );

	virtual		void		ControllerMove( int nSlot, float frametime, CUserCmd *cmd );
	virtual		void		MouseMove ( int nSlot, CUserCmd *cmd );

	virtual		void		UpdateMouseRotation();

	// Speed computation
	void					SetScrollTimeOut( bool forward, float sampleTime = 1.0f );
	void					CapAndSetSpeed( CUserCmd *cmd );
	
	// Binding switching
	void					GetBindings(const char *config, CUtlBuffer &bufout);
	void					SwitchConfig(const char *pSaveCurConfigTo, 
									const char *pSwitchToConfig, const char *pSwitchToConfigDefault);

	// Camera
	virtual		void		CAM_Think( void );
	virtual		void		CAM_GetCameraOffset( Vector& ofs );

	virtual float			WARS_GetCameraPitch( );
	virtual float			WARS_GetCameraYaw( );
	virtual float			WARS_GetCameraDist( );

	virtual void			Wars_CamReset();

	float					GetCurrentCamHeight() { return m_flCurrentCameraDist; }
	float					GetDesiredCamHeight() { return m_flDesiredCameraDist; }

	// Mouse clipping
	virtual		void		ActivateMouseClipping( void );
	virtual		void		DeactivateMouseClipping( void );

#ifdef WIN32
	// Windows
	void					BuildWindowList();
	void					AddWindow( HWND hwnd ) { m_Windows.AddToTail( hwnd ); }
	bool					HasWindowFocus();
	//HWND					GetWindow();
#endif // WIN32
	
private:
	float					m_fCurrentSampleTime;

	bool					m_bStrategicMode;
	bool					m_bMouseRotationActive;
	int						m_iLastPosX;
	int						m_iLastPosY;
	int						m_iDeltaX;
	int						m_iDeltaY;
	float					m_fLastPlayerCameraSettingsUpdateTime;


	// Middle mouse dragging
	bool					m_bWasMiddleMousePressed;
	Vector					m_vMiddleMouseStartPoint;
	Vector					m_vMiddleMousePlayerStartPoint;
	Vector					m_vMiddleMousePlayerDragSmoothed;

	// Scrolling
	bool				m_bScrollForward;
	float				m_fScrollTimeOut;
	bool				m_bScrolling;
	float				m_flDesiredCameraDist;
	float				m_flCurrentCameraDist;
	Vector				m_vecCameraVelocity;
	float				m_fLastOriginZ;

	// What is the current camera offset from the view origin?
	//Vector		m_vecCameraOffset;

#ifdef WIN32
	// Windows..
	CUtlVector< HWND > m_Windows;
#endif // WIN32
};

#ifdef WIN32
#ifdef USECUSTOMMOUSECLIPPING
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
inline HWND CHL2WarsInput::GetWindow()
{
	if( m_Windows.Count() > 0 )
		return m_Windows[0];
	return 0;
}
#endif // USECUSTOMMOUSECLIPPING
#endif // WIN32

//-----------------------------------------------------------------------------
// Purpose: Update the aim of the mouse pointer
//-----------------------------------------------------------------------------
inline void CHL2WarsInput::UpdateMouseAim( C_HL2WarsPlayer *pPlayer, int x, int y )
{
	Vector forward;
	CalculateMouseAim( pPlayer, x, y, forward );
	pPlayer->UpdateMouseData( forward );
}

#endif // WARS_IN_MAIN_H