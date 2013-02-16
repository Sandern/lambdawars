//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose: VGUI panel which can play back video, in-engine
//
//=============================================================================

#ifndef VGUI_VIDEO_GENERAL_H
#define VGUI_VIDEO_GENERAL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>
#include <vgui_controls/EditablePanel.h>
#include "avi/ibik.h"
#include "avi/iavi.h"

class VideoGeneralPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( VideoGeneralPanel, vgui::EditablePanel );
public:
	VideoGeneralPanel( vgui::Panel *pParent, const char *pPanelName, const char *pFileName=NULL );
	~VideoGeneralPanel();

	bool SetVideo( const char *pFileName );
	virtual void OnVideoOver( void );
	void StopVideo( void );

	void SetCropVideo( float fCropLeft, float fCropRight, float fCropTop, float fCropBottom );

	virtual void Paint( void );
	virtual void GetPanelPos( int &xpos, int &ypos );

	void SetVideoFlags( int flags );
	int GetVideoFlags();

private:
	bool CreateBINKVideo( const char *pFileName );
	bool CreateAVIVideo( const char *pFileName );

	void UpdateVideo( bool bForce = false );

private:
	char			m_VideoPath[MAX_PATH];
	bool			m_bUpdateVideoVariables;

	BIKMaterial_t	m_BIKHandle;
	AVIMaterial_t	m_AVIHandle;
	IMaterial		*m_pMaterial;

	int				m_nPlaybackHeight;			// Calculated to address ratio changes
	int				m_nPlaybackWidth;

	int				m_nCroppedPlaybackWidth;
	int				m_nCroppedPlaybackHeight;

	float			m_flU;	// U,V ranges for video on its sheet
	float			m_flV;

	// Used by avi
	int				m_iFrameNumber;
	float			m_fNextFrameUpdate;

	int m_iVideoFlags;

	// Cropping
	float m_fCropLeft, m_fCropRight, m_fCropTop, m_fCropBottom;
};

inline void VideoGeneralPanel::SetVideoFlags( int flags )
{
	m_iVideoFlags = flags;
}

inline int VideoGeneralPanel::GetVideoFlags()
{
	return m_iVideoFlags;
}

#endif // VGUI_VIDEO_GENERAL_H