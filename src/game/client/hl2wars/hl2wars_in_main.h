//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
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

#ifdef WIN32
	#include <winlite.h>
#endif // WIN32

class C_HL2WarsPlayer;

//-----------------------------------------------------------------------------
// Purpose: HL2Wars Input interface
//-----------------------------------------------------------------------------
class CHL2WarsInput : public CInput
{
public:
							CHL2WarsInput( void );

	virtual		void		Init_All( void );

	virtual void		LevelInit( void );

	virtual		void		CreateMove ( int sequence_number, float input_sample_frametime, bool active );
	virtual		void		ExtraMouseSample( float frametime, bool active );

	virtual		int			GetButtonBits( bool bResetState );

	virtual		void		AdjustAngles ( int nSlot, float frametime );

	virtual		void		ClampAngles( QAngle& viewangles );

	virtual		void		ResetMouse( void );

	virtual		void		GetFullscreenMousePos( int *mx, int *my, int *unclampedx = NULL, int *unclampedy = NULL );

	void					UpdateMouseAim( C_HL2WarsPlayer *pPlayer, int x, int y );

	virtual		void		ControllerMove( int nSlot, float frametime, CUserCmd *cmd );
	virtual		void		MouseMove ( int nSlot, CUserCmd *cmd );

	virtual		void		UpdateMouseRotation();

	// Speed computation
	void					ComputeMoveSpeed( float frametime );
	void					SetScrollTimeOut( bool forward );
	float					ComputeScrollSpeed( float frametime );
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

	// Windows
	void					BuildWindowList();
	void					AddWindow( HWND hwnd ) { m_Windows.AddToTail( hwnd ); }
	bool					HasWindowFocus();
	HWND					GetWindow();

private:
	float					m_fCurrentSampleTime;

	bool					m_bStrategicMode;
	bool					m_bMouseRotationActive;
	int						m_iLastPosX;
	int						m_iLastPosY;
	int						m_iDeltaX;
	int						m_iDeltaY;

	// Scrolling
	bool				m_bScrollForward;
	float				m_fScrollTimeOut;
	bool				m_bScrolling;
	bool				m_bResetDHNextSample;
	float				m_flDesiredCameraDist;
	float				m_flCurrentCameraDist;
	Vector				m_vecCameraVelocity;
	float				m_fLastOriginZ;

	// Times (for game instructor)
	float				m_fLastRotateTime;
	float				m_fLastZoomTime;
	float				m_fLastMoveTime;
	bool				m_bRotateHintActive;
	bool				m_bZoomHintActive;
	bool				m_bMoveHintActive;

	// What is the current camera offset from the view origin?
	//Vector		m_vecCameraOffset;

	// Windows..
	CUtlVector< HWND > m_Windows;
};

inline HWND CHL2WarsInput::GetWindow()
{
	if( m_Windows.Count() > 0 )
		return m_Windows[0];
	return 0;
}

#endif // WARS_IN_MAIN_H