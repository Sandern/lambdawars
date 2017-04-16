//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "src_cef_vgui_panel.h"
#include "src_cef_browser.h"
#include "src_cef.h"
#include "src_cef_osrenderer.h"
#include "clientmode_shared.h"

#include <vgui_controls/Controls.h>
#include <vgui/IInput.h>

// CEF
#include "include/cef_browser.h"

#include "../materialsystem/IShaderSystem.h"
#include "shaderapi/IShaderAPI.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

ConVar g_cef_draw("g_cef_draw", "1");
ConVar g_cef_debug_texture("g_cef_debug_texture", "0");
ConVar g_cef_loading_text_delay("g_cef_loading_text_delay", "20.0");

//-----------------------------------------------------------------------------
// Purpose: Find appropiate texture width/height helper
//-----------------------------------------------------------------------------
static int nexthigher( int k ) 
{
	k--;
	for (int i=1; i<sizeof(int)*CHAR_BIT; i<<=1)
			k = k | k >> i;
	return k+1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
SrcCefVGUIPanel::SrcCefVGUIPanel( const char *pName, SrcCefBrowser *pController, vgui::Panel *pParent ) 
	: Panel( NULL, "SrcCefPanel" ), m_pBrowser(pController), m_iTextureID(-1), 
	m_bTextureDirty(true), m_bPopupTextureDirty(false), m_bTextureGeneratedOnce(false)
{
	// WarsSplitscreen: only one player
	ACTIVE_SPLITSCREEN_PLAYER_GUARD( 0 );

	SetPaintBackgroundEnabled( false );
	SetScheme( "SourceScheme" );

	SetParent( pParent ? pParent : GetClientMode()->GetViewport() );

	m_Color = Color(255, 255, 255, 255);
	m_iTexWide = m_iTexTall = 0;

	m_iEventFlags = EVENTFLAG_NONE;
	m_iMouseX = 0;
	m_iMouseY = 0;

	m_bCalledLeftPressedParent = m_bCalledRightPressedParent = m_bCalledMiddlePressedParent = false;

	static int staticMatWebViewID = 0;
	V_snprintf( m_MatWebViewName, _MAX_PATH, "vgui/webview/webview_test%d", staticMatWebViewID++ );

	// Hack for working nice with VGUI input
	m_iTopZPos = 10;
	m_iBottomZPos = -15;
	m_bDontDraw = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
SrcCefVGUIPanel::~SrcCefVGUIPanel()
{
	m_pBrowser = NULL;

	if( m_iTextureID != -1 )
	{
		vgui::surface()->DestroyTextureID( m_iTextureID );
	}

	if( m_RenderBuffer.IsValid() )
	{
		m_RenderBuffer->SetTextureRegenerator( NULL );
		m_RenderBuffer.Shutdown();
	}
	if( m_MatRef.IsValid() )
	{
		m_MatRef.Shutdown();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool SrcCefVGUIPanel::ResizeTexture( int width, int height )
{
	if( !m_pBrowser )
		return false;

#ifdef USE_MULTITHREADED_MESSAGELOOP
	CefRefPtr<SrcCefOSRRenderer> renderer = m_pBrowser->GetOSRHandler();
	if( !renderer )
		return false;
#endif // USE_MULTITHREADED_MESSAGELOOP

	m_iWVWide = width;
	m_iWVTall = height;

	int po2wide = nexthigher(m_iWVWide);
	int po2tall = nexthigher(m_iWVTall);

	CefDbgMsg( 1, "Cef#%d: Resizing texture from %d %d to %d %d (wish size: %d %d)\n", GetBrowserID(), m_iTexWide, m_iTexTall, po2wide, po2tall, m_iWVWide, m_iWVTall );

	// Only rebuild when the actual size change and don't bother if the size
	// is larger than needed.
	// Otherwise just need to update m_fTexS1 and m_fTexT1 :)
	if( m_iTexWide >= po2wide && m_iTexTall >= po2tall )
	{
		m_fTexS1 = 1.0 - (m_iTexWide - m_iWVWide) / (float)m_iTexWide;
		m_fTexT1 = 1.0 - (m_iTexTall - m_iWVTall) / (float)m_iTexTall;
		return true;
	}

	m_iTexWide = po2wide;
	m_iTexTall = po2tall;
	m_fTexS1 = 1.0 - (m_iTexWide - m_iWVWide) / (float)m_iTexWide;
	m_fTexT1 = 1.0 - (m_iTexTall - m_iWVTall) / (float)m_iTexTall;

	if( m_RenderBuffer.IsValid() ) 
	{
		m_RenderBuffer.Shutdown();
	}
	if( m_MatRef.IsValid() )
	{
		m_MatRef.Shutdown();
	}

	static int staticTextureID = 0;
	V_snprintf( m_TextureWebViewName, _MAX_PATH, "_rt_test%d", staticTextureID++ );

	if( g_debug_cef.GetBool() )
		DevMsg("Cef: initializing texture %s\n", m_TextureWebViewName);

	// IMPORTANT: Use TEXTUREFLAGS_POINTSAMPLE. Otherwise mat_filtertextures will make it really blurred and ugly.
	// IMPORTANT 2: Use TEXTUREFLAGS_SINGLECOPY in case you want to be able to regenerate only a part of the texture (i.e. specify
	//				a sub rect when calling ->Download()).
	m_iTexImageFormat = IMAGE_FORMAT_BGRA8888;
	m_iTexFlags = TEXTUREFLAGS_PROCEDURAL|TEXTUREFLAGS_NOLOD|TEXTUREFLAGS_NOMIP|TEXTUREFLAGS_POINTSAMPLE|TEXTUREFLAGS_SINGLECOPY|
		TEXTUREFLAGS_PRE_SRGB|TEXTUREFLAGS_NODEPTHBUFFER;

	m_RenderBuffer.InitProceduralTexture( m_TextureWebViewName, TEXTURE_GROUP_VGUI, m_iTexWide, m_iTexTall, m_iTexImageFormat, m_iTexFlags);
	if( !m_RenderBuffer.IsValid() )
	{
		Warning("Cef: Failed to initialize render buffer texture\n");
		return false;
	}

	// Mark full dirty
	MarkTextureDirty();

	if( g_debug_cef.GetBool() )
		DevMsg("Cef: initializing material %s...\n", m_MatWebViewName);

	// Make sure the directory exists
	if( filesystem->FileExists("materials/vgui/webview", "MOD") == false )
	{
		filesystem->CreateDirHierarchy("materials/vgui/webview", "MOD");
	}

	// Create a material
	char MatBufName[_MAX_PATH];
	V_snprintf( MatBufName, _MAX_PATH, "materials/%s.vmt", m_MatWebViewName );
	KeyValues *pVMT = new KeyValues("WebView");
	pVMT->SetString("$basetexture", m_TextureWebViewName);
	pVMT->SetString("$translucent", "1");

#if 1
	// Save to file
	pVMT->SaveToFile( filesystem, MatBufName, "MOD" );

	m_MatRef.Init( m_MatWebViewName, TEXTURE_GROUP_VGUI, true );
	materials->ReloadMaterials( m_MatWebViewName );
#else
	// TODO: Figure out how to use the following instead of writing to file:
	//		 This should init it as a procedural material, but it doesn't seems to work...
	m_MatRef.Init(m_MatWebViewName, TEXTURE_GROUP_VGUI, pVMT);
#endif // 1

	pVMT->deleteThis();

	CefDbgMsg( 1, "Cef#%d: Finished initializing web material.\n", GetBrowserID() );

	// Update texture ID
	CefDbgMsg( 1, "Cef#%d: initializing texture id...\n", GetBrowserID() );

	if( m_iTextureID != -1 )
	{
		vgui::surface()->DestroyTextureID( m_iTextureID );
	}

	m_iTextureID = vgui::surface()->CreateNewTextureID( true );
	vgui::surface()->DrawSetTextureFile( m_iTextureID, m_MatWebViewName, false, false );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefVGUIPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_hLoadingFont = pScheme->GetFont( "HUDNumber" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefVGUIPanel::OnThink()
{
	BaseClass::OnThink();

	if( !m_pBrowser )
		return;

	if( m_pBrowser->GetPassMouseTruIfAlphaZero() )
	{
		// Hack for working nice with VGUI input
		// Move to back in case the mouse is on nothing
		// Move to front in case on something
		int x, y;
		vgui::input()->GetCursorPos( x, y );
		ScreenToLocal( x, y );

		if( m_pBrowser->IsAlphaZeroAt( x, y ) )
		{
			SetZPos( m_iBottomZPos );
		}
		else
		{
			SetZPos( m_iTopZPos );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefVGUIPanel::Paint()
{
	CefRefPtr<SrcCefOSRRenderer> renderer = m_pBrowser->GetOSRHandler();
	if( renderer )
	{
#ifdef USE_MULTITHREADED_MESSAGELOOP
		AUTO_LOCK( renderer->GetTextureBufferMutex() );
#endif // USE_MULTITHREADED_MESSAGELOOP

		if( renderer->GetWidth() != m_iWVWide || renderer->GetHeight() != m_iWVTall )
		{
			if( !ResizeTexture( renderer->GetWidth(), renderer->GetHeight() ) )
				return;
		}

		SetCursor( renderer->GetCursor() );

		if( g_pShaderAPI && m_RenderBuffer.IsValid() )
		{
			VPROF_BUDGET( "Upload", "CefUploadTexture" );

			int nFrame = 0;
			int nTextureChannel = 0;
			ShaderAPITextureHandle_t textureHandle = g_pSLShaderSystem->GetShaderAPITextureBindHandle( m_RenderBuffer, nFrame, nTextureChannel );
			if( g_pShaderAPI->IsTexture( textureHandle ) )
			{
				int iFace = 0;
				int iMip = 0;
				int z = 0;

				// Material system might be modifying textures too in a different thread, so lock the texture
				MaterialLock_t lock = NULL;

				if( renderer->GetTextureBuffer() && m_bTextureDirty )
				{
					lock = materials->Lock();

#ifndef RENDER_DIRTY_AREAS
					// Full Copy
					g_pShaderAPI->ModifyTexture( textureHandle );
					g_pShaderAPI->TexImage2D( iMip, iFace, m_iTexImageFormat, z, renderer->GetWidth(), renderer->GetHeight(), m_iTexImageFormat, false, renderer->GetTextureBuffer() );
#else
					// Partial copy, would work if we had a partial buffer:
					//g_pShaderAPI->TexSubImage2D( 0 /* level */, 0 /* cubeFaceID */, m_iDirtyX, m_iDirtyY, 0 /* zoffset */, 
					//	(m_iDirtyXEnd - m_iDirtyX), (m_iDirtyYEnd - m_iDirtyY), m_iTexImageFormat, 0, false,  pSubBuffer );
					// Copy by line, but call too expensive to do per line:
					int subWidth = m_iDirtyXEnd - m_iDirtyX;
					for( int y = m_iDirtyY; y < m_iDirtyYEnd; y++ )
					{
						g_pShaderAPI->TexSubImage2D( 0, 0, m_iDirtyX, y, z, 
							subWidth, 1, m_iTexImageFormat, 0, false, 
							renderer->GetTextureBuffer() + (y * 4 * renderer->GetWidth()) + (m_iDirtyX * 4) );
					}
#endif // RENDER_DIRTY_AREAS

					m_bTextureDirty = false;
					m_bTextureGeneratedOnce = true;
#ifdef RENDER_DIRTY_AREAS
					m_iDirtyX = renderer->GetWidth();
					m_iDirtyY = renderer->GetHeight();
					m_iDirtyXEnd = m_iDirtyYEnd = 0;
#endif // RENDER_DIRTY_AREAS
				}

				if( renderer->GetPopupBuffer() && m_bPopupTextureDirty )
				{
					if( lock == NULL ) {
						lock = materials->Lock();
					}
					g_pShaderAPI->ModifyTexture( textureHandle );
					g_pShaderAPI->TexSubImage2D( iMip, iFace, renderer->GetPopupOffsetX(), renderer->GetPopupOffsetY(), z, renderer->GetPopupWidth(), renderer->GetPopupHeight(),
						m_iTexImageFormat, 0, false, renderer->GetPopupBuffer() );

					m_bPopupTextureDirty = false;
				}

				// Always release the lock
				if( lock )
				{
					materials->Unlock( lock );
				}
			}
		}
	}

	// Must have a valid texture and the texture regenerator should be valid
	if( !m_bDontDraw )
	{
		DrawWebview();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefVGUIPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	CefRefPtr<SrcCefOSRRenderer> renderer = m_pBrowser->GetOSRHandler();
	if( !renderer )
		return;

	int x, y, wide, tall;
	GetPos( x, y );
	GetSize( wide, tall );

	renderer->UpdateViewRect( x, y, wide, tall );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefVGUIPanel::DrawWebview()
{
	int iWide, iTall;
	GetSize( iWide, iTall );

	if( surface()->IsTextureIDValid( m_iTextureID ) && m_bTextureGeneratedOnce && g_cef_draw.GetBool() )
	{
		vgui::surface()->DrawSetColor( m_Color );
		vgui::surface()->DrawSetTexture( m_iTextureID );
		vgui::surface()->DrawTexturedSubRect( 0, 0, iWide, iTall, 0, 0, m_fTexS1, m_fTexT1 );
	}
	else
	{
		// Show loading text if it takes longer than a certain time
		if (Plat_FloatTime() - m_pBrowser->GetLastLoadStartTime() > g_cef_loading_text_delay.GetFloat()) 
		{
			wchar_t buf[256];
			mbstowcs( buf, m_pBrowser->GetName(), sizeof(buf) );

			wchar_t text[256];
			V_snwprintf( text, sizeof( text ) / sizeof(wchar_t), L"Loading %ls...", buf );

			int iTextWide, iTextTall;
			surface()->GetTextSize( m_hLoadingFont, text, iTextWide, iTextTall );

			vgui::surface()->DrawSetTextFont( m_hLoadingFont );
			vgui::surface()->DrawSetTextColor( m_Color );
			vgui::surface()->DrawSetTextPos( (iWide / 2) - (iTextWide / 2), (iTall / 2) - (iTextTall / 2) );

			vgui::surface()->DrawUnicodeString( text );
		}

		// Debug reason why nothing is showing yet
		if( g_cef_debug_texture.GetBool() )
		{
			DevMsg("CEF: not drawing");
			if( m_iTextureID == -1 )
				DevMsg(", texture does not exists");
			if( m_bTextureDirty )
				DevMsg(", texture is dirty");
			if( !g_cef_draw.GetBool() )
				DevMsg(", g_cef_draw is 0");
			DevMsg("\n");
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	SrcCefVGUIPanel::GetEventFlags()
{
	return m_iEventFlags | CEFSystem().GetKeyModifiers();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefVGUIPanel::OnCursorEntered()
{
	if( !IsValid() )
		return;

	// Called before OnCursorMoved, so mouse coordinates might be outdated
	int x, y;
	vgui::input()->GetCursorPos( x, y );
	ScreenToLocal( x, y );

	m_iMouseX = x;
	m_iMouseY = y;

	CefMouseEvent me;
	me.x = m_iMouseX;
	me.y = m_iMouseY;
	me.modifiers = GetEventFlags();
	m_pBrowser->GetBrowser()->GetHost()->SendMouseMoveEvent( me, false );

	CefDbgMsg( 2, "Cef#%d: injected cursor entered %d %d\n", GetBrowserID(), m_iMouseX, m_iMouseY );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefVGUIPanel::OnCursorExited()
{
	if( !IsValid() )
		return;

	CefMouseEvent me;
	me.x = m_iMouseX;
	me.y = m_iMouseY;
	me.modifiers = GetEventFlags();
	m_pBrowser->GetBrowser()->GetHost()->SendMouseMoveEvent( me, true );

	CefDbgMsg( 2, "Cef#%d: injected cursor exited %d %d\n", GetBrowserID(), m_iMouseX, m_iMouseY );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefVGUIPanel::OnCursorMoved( int x, int y )
{
	if( !IsValid() )
		return;

	m_iMouseX = x;
	m_iMouseY = y;

	if( m_pBrowser->GetPassMouseTruIfAlphaZero() && m_pBrowser->IsAlphaZeroAt( x, y ) )
	{
		CefDbgMsg( 3, "Cef#%d: passed cursor move %d %d to parent (alpha zero)\n", GetBrowserID(), x, y );

		CallParentFunction(new KeyValues("OnCursorMoved", "x", x, "y", y));
	}

	CefMouseEvent me;
	me.x = x;
	me.y = y;
	me.modifiers = GetEventFlags();
	m_pBrowser->GetBrowser()->GetHost()->SendMouseMoveEvent( me, false );

	CefDbgMsg( 3, "Cef#%d: injected cursor move %d %d\n", GetBrowserID(), x, y );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefVGUIPanel::UpdatePressedParent( vgui::MouseCode code, bool state )
{
	switch( code )
	{
	case MOUSE_LEFT:
		m_bCalledLeftPressedParent = state;
		break;
	case MOUSE_RIGHT:
		m_bCalledRightPressedParent = state;
		break;
	case MOUSE_MIDDLE:
		m_bCalledMiddlePressedParent = state;
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool SrcCefVGUIPanel::IsPressedParent( vgui::MouseCode code )
{
	switch( code )
	{
	case MOUSE_LEFT:
		return m_bCalledLeftPressedParent;
	case MOUSE_RIGHT:
		return m_bCalledRightPressedParent;
	case MOUSE_MIDDLE:
		return m_bCalledMiddlePressedParent;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefVGUIPanel::OnMousePressed(vgui::MouseCode code)
{
	if( !IsValid() )
		return;

	// Do click on parent if needed
	// Store if we pressed on the parent
	if( m_pBrowser->GetPassMouseTruIfAlphaZero() && m_pBrowser->IsAlphaZeroAt( m_iMouseX, m_iMouseY ) )
	{
		CefDbgMsg( 1, "Cef#%d: passed mouse pressed %d %d to parent\n", GetBrowserID(), m_iMouseX, m_iMouseY );

		CallParentFunction(new KeyValues("MousePressed", "code", code));

		UpdatePressedParent( code, true );
	}
	else
	{
		UpdatePressedParent( code, false );
	}

	if( m_pBrowser->GetUseMouseCapture() )
	{
		// Make sure released is called on this panel
		// Make sure to do this after calling the parent, so we override
		// the mouse capture to this panel
		vgui::input()->SetMouseCaptureEx(GetVPanel(), code);
	}

	CefBrowserHost::MouseButtonType iMouseType = MBT_LEFT;

	CefMouseEvent me;
	me.x = m_iMouseX;
	me.y = m_iMouseY;

	switch( code )
	{
	case MOUSE_LEFT:
		iMouseType = MBT_LEFT;
		m_iEventFlags |= EVENTFLAG_LEFT_MOUSE_BUTTON;
		break;
	case MOUSE_RIGHT:
		iMouseType = MBT_RIGHT;
		m_iEventFlags |= EVENTFLAG_RIGHT_MOUSE_BUTTON;
		break;
	case MOUSE_MIDDLE:
		iMouseType = MBT_MIDDLE;
		m_iEventFlags |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
		break;
	}

	me.modifiers = GetEventFlags();

	m_pBrowser->GetBrowser()->GetHost()->SendMouseClickEvent( me, iMouseType, false, 1 );

	CefDbgMsg( 1, "Cef#%d: injected mouse pressed %d %d (mouse capture: %d)\n", GetBrowserID(), m_iMouseX, m_iMouseY, m_pBrowser->GetUseMouseCapture() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefVGUIPanel::OnMouseDoublePressed(vgui::MouseCode code)
{
	if( !IsValid() )
		return;

	// Do click on parent if needed
	if( m_pBrowser->GetPassMouseTruIfAlphaZero() && m_pBrowser->IsAlphaZeroAt( m_iMouseX, m_iMouseY ) )
	{
		CefDbgMsg( 1, "Cef#%d: passed mouse double pressed %d %d to parent\n", GetBrowserID(), m_iMouseX, m_iMouseY );

		CallParentFunction(new KeyValues("MouseDoublePressed", "code", code));
	}

	CefBrowserHost::MouseButtonType iMouseType = MBT_LEFT;

	// Do click
	CefMouseEvent me;
	me.x = m_iMouseX;
	me.y = m_iMouseY;

	switch( code )
	{
	case MOUSE_LEFT:
		iMouseType = MBT_LEFT;
		m_iEventFlags |= EVENTFLAG_LEFT_MOUSE_BUTTON;
		break;
	case MOUSE_RIGHT:
		iMouseType = MBT_RIGHT;
		m_iEventFlags |= EVENTFLAG_RIGHT_MOUSE_BUTTON;
		break;
	case MOUSE_MIDDLE:
		iMouseType = MBT_MIDDLE;
		m_iEventFlags |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
		break;
	}

	me.modifiers = GetEventFlags();

	m_pBrowser->GetBrowser()->GetHost()->SendMouseClickEvent( me, iMouseType, false, 2 );

	CefDbgMsg( 1, "Cef#%d: injected mouse double pressed %d %d\n", GetBrowserID(), m_iMouseX, m_iMouseY );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefVGUIPanel::OnMouseReleased(vgui::MouseCode code)
{	
	if( !IsValid() )
		return;

	// Check mouse capture and make sure it is cleared
	bool bHasMouseCapture = vgui::input()->GetMouseCapture() == GetVPanel();
	if( bHasMouseCapture )
	{
		vgui::input()->SetMouseCaptureEx( 0, code );
	}

	// Check if we should pass input to parent (only if we don't have mouse capture active)
	if( (m_pBrowser->GetUseMouseCapture() && IsPressedParent( code )) || ( m_pBrowser->GetPassMouseTruIfAlphaZero() && m_pBrowser->IsAlphaZeroAt( m_iMouseX, m_iMouseY ) ) )
	{
		CefDbgMsg( 1, "Cef#%d: passed mouse released %d %d to parent\n", GetBrowserID(), m_iMouseX, m_iMouseY );

		CallParentFunction(new KeyValues("MouseReleased", "code", code));
	}

	// Clear parent pressed
	UpdatePressedParent( code, false );

	// Do click
	CefBrowserHost::MouseButtonType iMouseType = MBT_LEFT;

	CefMouseEvent me;
	me.x = m_iMouseX;
	me.y = m_iMouseY;

	switch( code )
	{
	case MOUSE_LEFT:
		iMouseType = MBT_LEFT;
		m_iEventFlags &= ~EVENTFLAG_LEFT_MOUSE_BUTTON;
		break;
	case MOUSE_RIGHT:
		iMouseType = MBT_RIGHT;
		m_iEventFlags &= ~EVENTFLAG_RIGHT_MOUSE_BUTTON;
		break;
	case MOUSE_MIDDLE:
		iMouseType = MBT_MIDDLE;
		m_iEventFlags &= ~EVENTFLAG_MIDDLE_MOUSE_BUTTON;
		break;
	}

	me.modifiers = GetEventFlags();

	m_pBrowser->GetBrowser()->GetHost()->SendMouseClickEvent( me, iMouseType, true, 1 );

	CefDbgMsg( 1, "Cef#%d: injected mouse released %d %d\n", GetBrowserID(), m_iMouseX, m_iMouseY );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefVGUIPanel::OnMouseWheeled( int delta )
{
	if( !IsValid() )
		return;

	if( m_pBrowser->GetPassMouseTruIfAlphaZero() && m_pBrowser->IsAlphaZeroAt( m_iMouseX, m_iMouseY ) )
	{
		CallParentFunction(new KeyValues("MouseWheeled", "delta", delta));
		return;
	}

	CefDbgMsg( 1, "Cef#%d: injected mouse wheeled %d\n", GetBrowserID(), delta );

	CefMouseEvent me;
	me.x = m_iMouseX;
	me.y = m_iMouseY;
	me.modifiers = GetEventFlags();

	// VGUI just gives -1 or +1. SendMouseWheelEvent expects the number of pixels to shift.
	// Use the last mouse wheel value from the window proc instead
	m_pBrowser->GetBrowser()->GetHost()->SendMouseWheelEvent( me, 0, CEFSystem().GetLastMouseWheelDist() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
vgui::HCursor SrcCefVGUIPanel::GetCursor()
{
	if( GetParent() && m_pBrowser->GetPassMouseTruIfAlphaZero() && m_pBrowser->IsAlphaZeroAt( m_iMouseX, m_iMouseY ) )
	{
		return GetParent()->GetCursor();
	}

	return BaseClass::GetCursor();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	SrcCefVGUIPanel::GetBrowserID()
{
	return m_pBrowser && m_pBrowser->GetBrowser() ? m_pBrowser->GetBrowser()->GetIdentifier() : -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool SrcCefVGUIPanel::IsValid()
{
	return m_pBrowser && m_pBrowser->GetBrowser();
}
