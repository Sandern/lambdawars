//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose: VGUI panel which can play back video, in-engine
//
//=============================================================================

#include "cbase.h"
#include <vgui/IVGui.h>
#include "vgui/IInput.h"
#include <vgui/ISurface.h>
#include "ienginevgui.h"
#include "iclientmode.h"
#include "vgui_video_general.h"
#include "engine/ienginesound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
VideoGeneralPanel::VideoGeneralPanel( vgui::Panel *pParent, const char *pPanelName, const char *pFileName )
		: BaseClass( pParent, pPanelName ),
		m_BIKHandle( BIKHANDLE_INVALID ),
		m_AVIHandle( AVIMATERIAL_INVALID ),
		m_nPlaybackWidth( 0 ),
		m_nPlaybackHeight( 0 ),
		m_iVideoFlags(0),
		m_fCropLeft(0.0f),
		m_fCropRight(0.0f),
		m_fCropTop(0.0f),
		m_fCropBottom(0.0f),
		m_pMaterial(NULL)
{
	m_VideoPath[0] = '\0';

	SetProportional( false );
	SetVisible( true );
	SetPaintBackgroundEnabled( false );
	SetPaintBorderEnabled( false );

	if( pFileName )
		SetVideo( pFileName );

	SetScheme(vgui::scheme()->LoadSchemeFromFile( "resource/VideoPanelScheme.res", "VideoPanelScheme"));
	LoadControlSettings("resource/UI/VideoPanel.res");
}

//-----------------------------------------------------------------------------
// Properly shutdown out materials
//-----------------------------------------------------------------------------
VideoGeneralPanel::~VideoGeneralPanel( void )
{
	StopVideo();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool VideoGeneralPanel::SetVideo( const char *pFileName )
{
	// Destroy any previously allocated video
	StopVideo();

	Q_strcpy( m_VideoPath, pFileName );

	// Filename
	char strExtension[8];

	Q_ExtractFileExtension(pFileName, strExtension, 8);

	m_bUpdateVideoVariables = true;

	// Create the video
	if( !Q_strncmp(strExtension, "bik", 3) )
	{
		return CreateBINKVideo(pFileName);
	}
	else if( !Q_strncmp(strExtension, "avi", 3) )
	{
		return CreateAVIVideo(pFileName);
	}
	else
	{
		// Assume no extension and and assume bik
		char strFullpath[MAX_PATH];
		Q_strncat( strFullpath, pFileName, MAX_PATH );
		Q_strncat( strFullpath, ".bik", MAX_PATH );		// Assume we're a .bik extension type

		return CreateBINKVideo(strFullpath);
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool VideoGeneralPanel::CreateBINKVideo( const char *pFileName )
{
	// Load and create our BINK video
	char szMaterialName[ MAX_PATH ];
	Q_snprintf( szMaterialName, sizeof( szMaterialName ), "PortraitBIKMaterial%i", g_pBIK->GetGlobalMaterialAllocationNumber() );
	m_BIKHandle = bik->CreateMaterial( szMaterialName, pFileName, "GAME", m_iVideoFlags );
	if ( m_BIKHandle == BIKHANDLE_INVALID )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VideoGeneralPanel::UpdateVideo( bool bForce )
{
	if( !bForce && !m_bUpdateVideoVariables )
		return;

	if ( m_BIKHandle == BIKHANDLE_INVALID )
		return;

	int nWidth, nHeight;
	bik->GetFrameSize( m_BIKHandle, &nWidth, &nHeight );
	bik->GetTexCoordRange( m_BIKHandle, &m_flU, &m_flV );
	m_pMaterial = bik->GetMaterial( m_BIKHandle );

	//float flFrameRatio = ( (float) GetWide() / (float) GetTall() );
	//float flVideoRatio = ( (float) nWidth / (float) nHeight );

	m_nPlaybackWidth = GetWide();
	m_nPlaybackHeight = GetTall();

	m_nCroppedPlaybackWidth = m_nPlaybackWidth * (1.0f - (m_fCropLeft+m_fCropRight));
	m_nCroppedPlaybackHeight = m_nPlaybackHeight * (1.0f - (m_fCropTop+m_fCropBottom));

	/*
	if ( flVideoRatio > flFrameRatio )
	{
		m_nPlaybackWidth = GetWide();
		m_nPlaybackHeight = ( GetWide() / flVideoRatio );
	}
	else if ( flVideoRatio < flFrameRatio )
	{
		m_nPlaybackWidth = ( GetTall() * flVideoRatio );
		m_nPlaybackHeight = GetTall();
	}
	else
	{
		m_nPlaybackWidth = GetWide();
		m_nPlaybackHeight = GetTall();
	}


*/

	m_bUpdateVideoVariables = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool VideoGeneralPanel::CreateAVIVideo( const char *pFileName )
{	
	// Load and create our BINK video
	m_AVIHandle = avi->CreateAVIMaterial( "VideoAVIMaterial", pFileName, "GAME" );
	if ( m_AVIHandle == AVIMATERIAL_INVALID )
		return false;

	int nWidth, nHeight;
	avi->GetFrameSize( m_AVIHandle, &nWidth, &nHeight );
	avi->GetTexCoordRange( m_AVIHandle, &m_flU, &m_flV );
	m_pMaterial = avi->GetMaterial( m_AVIHandle );

	float flFrameRatio = ( (float) GetWide() / (float) GetTall() );
	float flVideoRatio = ( (float) nWidth / (float) nHeight );

	if ( flVideoRatio > flFrameRatio )
	{
		m_nPlaybackWidth = GetWide();
		m_nPlaybackHeight = ( GetWide() / flVideoRatio );
	}
	else if ( flVideoRatio < flFrameRatio )
	{
		m_nPlaybackWidth = ( GetTall() * flVideoRatio );
		m_nPlaybackHeight = GetTall();
	}
	else
	{
		m_nPlaybackWidth = GetWide();
		m_nPlaybackHeight = GetTall();
	}

	m_iFrameNumber = 0;
	m_fNextFrameUpdate = 0.0f;

	m_nCroppedPlaybackWidth = m_nPlaybackWidth * (1.0f - (m_fCropLeft+m_fCropRight));
	m_nCroppedPlaybackHeight = m_nPlaybackHeight * (1.0f - (m_fCropTop+m_fCropBottom));

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VideoGeneralPanel::OnVideoOver()
{
	if ( m_BIKHandle != BIKHANDLE_INVALID )
	{
		bik->SetFrame( m_BIKHandle, 0 );
	}
	else if ( m_AVIHandle != AVIMATERIAL_INVALID ) 
	{
		avi->SetFrame(m_AVIHandle, 0);
		m_iFrameNumber = 0;
		m_fNextFrameUpdate = gpGlobals->curtime + ( 1.0f / avi->GetFrameRate(m_AVIHandle) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VideoGeneralPanel::StopVideo()
{
	// Shut down this video
	if ( m_BIKHandle != BIKHANDLE_INVALID )
	{
		bik->DestroyMaterial( m_BIKHandle );
		m_BIKHandle = BIKHANDLE_INVALID;
	}
	else if ( m_AVIHandle != AVIMATERIAL_INVALID )
	{
		avi->DestroyAVIMaterial( m_AVIHandle );
		m_AVIHandle = AVIMATERIAL_INVALID;
	}
	m_pMaterial = NULL;
	m_VideoPath[0] = '\0';
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VideoGeneralPanel::SetCropVideo( float fCropLeft, float fCropRight, float fCropTop, float fCropBottom )
{
	m_fCropLeft = fCropLeft;
	m_fCropRight = fCropRight;
	m_fCropTop = fCropTop;
	m_fCropBottom = fCropBottom;

	m_nCroppedPlaybackWidth = m_nPlaybackWidth * (1.0f - (m_fCropLeft+m_fCropRight));
	m_nCroppedPlaybackHeight = m_nPlaybackHeight * (1.0f - (m_fCropTop+m_fCropBottom));

	m_bUpdateVideoVariables = true;
}

//-----------------------------------------------------------------------------
// Purpose: Update and draw the frame
//-----------------------------------------------------------------------------
void VideoGeneralPanel::Paint( void )
{
	//BaseClass::Paint();

	UpdateVideo();
	
	if ( m_BIKHandle != BIKHANDLE_INVALID )
	{
		if ( g_pBIK->ReadyForSwap( m_BIKHandle ) )
		{
			// Update our frame
			if ( bik->Update( m_BIKHandle ) == false )
			{
				// Issue a close command
				OnVideoOver();
			}
		}
	}
	else if(  m_AVIHandle != AVIMATERIAL_INVALID ) 
	{
		if( m_iFrameNumber == avi->GetFrameCount( m_AVIHandle ) )
		{
			// Issue a close command
			OnVideoOver();
		}
		else
		{
			if( m_fNextFrameUpdate < gpGlobals->curtime )
			{
				m_iFrameNumber++;
				avi->SetFrame(m_AVIHandle, m_iFrameNumber);
				m_fNextFrameUpdate = gpGlobals->curtime + ( 1.0f / avi->GetFrameRate(m_AVIHandle) );
			}
		}
		
	}
	else
	{
		// No video to play, so do nothing
		return;
	}	

	if( !m_pMaterial )
		return;

	// Sit in the "center"
	int xpos, ypos;
	GetPanelPos( xpos, ypos );

	// Black out the background (we could omit drawing under the video surface, but this is straight-forward)
	/*if ( m_bBlackBackground )
	{
		vgui::surface()->DrawSetColor(  0, 0, 0, 255 );
		vgui::surface()->DrawFilledRect( 0, 0, GetWide(), GetTall() );
	}*/

	// Draw the polys to draw this out
	CMatRenderContextPtr pRenderContext( materials );
	
	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	pRenderContext->Bind( m_pMaterial, NULL );

	CMeshBuilder meshBuilder;
	IMesh* pMesh = pRenderContext->GetDynamicMesh( true );
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	float flLeftX = xpos;
	float flRightX = xpos + (m_nPlaybackWidth-1);

	float flTopY = ypos;
	float flBottomY = ypos + (m_nPlaybackHeight-1);

	// Map our UVs to cut out just the portion of the video we're interested in
	float flLeftU = 0.0f;
	float flTopV = 0.0f;

	// We need to subtract off a pixel to make sure we don't bleed
	float flRightU = m_flU - ( 1.0f / (float) m_nCroppedPlaybackWidth );
	float flBottomV = m_flV - ( 1.0f / (float) m_nCroppedPlaybackHeight );

	flLeftU = flRightU * m_fCropLeft;
	flRightU *= (1.0f-m_fCropRight);
	flTopV = flBottomV * m_fCropTop;
	flBottomV *= (1.0f-m_fCropBottom);

	// Get the current viewport size
	int vx, vy, vw, vh;
	pRenderContext->GetViewport( vx, vy, vw, vh );

	// map from screen pixel coords to -1..1
	flRightX = FLerp( -1, 1, 0, vw, flRightX );
	flLeftX = FLerp( -1, 1, 0, vw, flLeftX );
	flTopY = FLerp( 1, -1, 0, vh ,flTopY );
	flBottomY = FLerp( 1, -1, 0, vh, flBottomY );

	float alpha = ((float)GetFgColor()[3]/255.0f);

	for ( int corner=0; corner<4; corner++ )
	{
		bool bLeft = (corner==0) || (corner==3);
		meshBuilder.Position3f( (bLeft) ? flLeftX : flRightX, (corner & 2) ? flBottomY : flTopY, 0.0f );
		meshBuilder.Normal3f( 0.0f, 0.0f, 1.0f );
		meshBuilder.TexCoord2f( 0, (bLeft) ? flLeftU : flRightU, (corner & 2) ? flBottomV : flTopV );
		meshBuilder.TangentS3f( 0.0f, 1.0f, 0.0f );
		meshBuilder.TangentT3f( 1.0f, 0.0f, 0.0f );
		meshBuilder.Color4f( 1.0f, 1.0f, 1.0f, alpha );
		meshBuilder.AdvanceVertex();
	}
	
	meshBuilder.End();
	pMesh->Draw();

	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PopMatrix();

	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PopMatrix();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VideoGeneralPanel::GetPanelPos( int &xpos, int &ypos )
{
	xpos = ( (float) ( GetWide() - m_nPlaybackWidth ) / 2 );
	ypos = ( (float) ( GetTall() - m_nPlaybackHeight ) / 2 );
	LocalToScreen(xpos, ypos);
}