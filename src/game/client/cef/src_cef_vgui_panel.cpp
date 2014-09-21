//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "src_cef_vgui_panel.h"
#include "src_cef_browser.h"
#include "src_cef.h"
#include "src_cef_texgen.h"
#include "src_cef_osrenderer.h"
#include "clientmode_shared.h"

#include <vgui_controls/Controls.h>
#include <vgui/IInput.h>

// CEF
#include "include/cef_browser.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

ConVar g_cef_draw("g_cef_draw", "1");
ConVar g_cef_debug_texture("g_cef_debug_texture", "0");

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
	: Panel( NULL, "SrcCefPanel" ), m_pBrowser(pController), m_iTextureID(-1), m_bSizeChanged(false), 
		m_pTextureRegen(NULL)
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

	m_iDirtyX = m_iDirtyY = m_iDirtyXEnd = m_iDirtyYEnd = 0;

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
	if( m_pTextureRegen )
	{
		delete m_pTextureRegen; 
		m_pTextureRegen = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool SrcCefVGUIPanel::ResizeTexture( int width, int height )
{
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

	if( !m_pTextureRegen )
	{
		if( g_debug_cef.GetBool() )
			DevMsg("Cef: initializing texture regenerator\n");
		m_pTextureRegen = new CCefTextureGenerator( m_pBrowser );
	}
	else
	{
		if( g_debug_cef.GetBool() )
			DevMsg("Cef: Invalidating old render texture\n");
		m_RenderBuffer->SetTextureRegenerator( NULL );
		m_RenderBuffer.Shutdown();
		m_MatRef.Shutdown();
	}

	static int staticTextureID = 0;
	V_snprintf( m_TextureWebViewName, _MAX_PATH, "_rt_test%d", staticTextureID++ );

	if( g_debug_cef.GetBool() )
		DevMsg("Cef: initializing texture %s\n", m_TextureWebViewName);

	// IMPORTANT: Use TEXTUREFLAGS_POINTSAMPLE. Otherwise mat_filtertextures will make it really blurred and ugly.
	// IMPORTANT 2: Use TEXTUREFLAGS_SINGLECOPY in case you want to be able to regenerate only a part of the texture (i.e. specify
	//				a sub rect when calling ->Download()).
	m_RenderBuffer.InitProceduralTexture( m_TextureWebViewName, TEXTURE_GROUP_VGUI, m_iTexWide, m_iTexTall, IMAGE_FORMAT_BGRA8888, 
		TEXTUREFLAGS_PROCEDURAL|TEXTUREFLAGS_NOLOD|TEXTUREFLAGS_NOMIP|TEXTUREFLAGS_POINTSAMPLE|TEXTUREFLAGS_SINGLECOPY|
		TEXTUREFLAGS_PRE_SRGB|TEXTUREFLAGS_NODEPTHBUFFER);
	if( !m_RenderBuffer.IsValid() )
	{
		Warning("Cef: Failed to initialize render buffer texture\n");
		return false;
	}

	m_RenderBuffer->SetTextureRegenerator( m_pTextureRegen );

	// Mark full dirty
	MarkTextureDirty( 0, 0, m_iWVWide, m_iWVTall );

	//if( m_MatRef.IsValid() )
	//	m_MatRef.Shutdown();

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
	//pVMT->SetString("$no_fullbright", "1");
	//pVMT->SetString("$ignorez", "1");

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
	int iWide, iTall;
	GetSize( iWide, iTall );

	// Update texture if needed
	if( m_bSizeChanged )
	{
		// Update texture
		if( !ResizeTexture( iWide, iTall ) )
			return;

		m_bSizeChanged = false;

		// Update texture ID
		CefDbgMsg( 1, "Cef#%d: initializing texture id...\n", GetBrowserID() );

		if( m_iTextureID != -1 )
		{
			vgui::surface()->DestroyTextureID( m_iTextureID );
		}

		m_iTextureID = vgui::surface()->CreateNewTextureID( true );
		vgui::surface()->DrawSetTextureFile( m_iTextureID, m_MatWebViewName, false, false );
	}

	if( !m_pTextureRegen )
		return;

	// Regenerate texture (partially) if needed
	if( m_pTextureRegen->IsDirty() )
	{
		//if( g_cef_debug_texture.GetBool() )
		//	DevMsg("CEF: texture requires full regeneration\n");
		m_RenderBuffer->SetTextureRegenerator( m_pTextureRegen );
		{
			VPROF_BUDGET( "Download", "CefDownloadTexture" );
			Rect_t dirtyArea;
			dirtyArea.x = m_iDirtyX;
			dirtyArea.y = m_iDirtyY;
			dirtyArea.width = m_iDirtyXEnd - m_iDirtyX;
			dirtyArea.height = m_iDirtyYEnd - m_iDirtyY;
			m_RenderBuffer->Download( &dirtyArea );
		}

		// Clear if no longer dirty
		if( !m_pTextureRegen->IsDirty() )
		{
			m_iDirtyX = m_iWVWide;
			m_iDirtyY = m_iWVTall;
			m_iDirtyXEnd = m_iDirtyYEnd = 0;
		}

		//if( m_pTextureRegen->IsDirty() == false )
		//	materials->ReloadMaterials(m_MatWebViewName); // FIXME
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
void SrcCefVGUIPanel::DrawWebview()
{
	int iWide, iTall;
	GetSize( iWide, iTall );

	if( m_iTextureID != -1 && m_pTextureRegen->IsDirty() == false && g_cef_draw.GetBool() )
	{
		vgui::surface()->DrawSetColor( m_Color );
		vgui::surface()->DrawSetTexture( m_iTextureID );
		vgui::surface()->DrawTexturedSubRect( 0, 0, iWide, iTall, 0, 0, m_fTexS1, m_fTexT1 );
	}
	else
	{
		// Show loading text
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

		// Debug reason
		if( g_cef_debug_texture.GetBool() )
		{
			DevMsg("CEF: not drawing");
			if( m_iTextureID == -1 )
				DevMsg(", texture does not exists");
			if( m_pTextureRegen->IsDirty() )
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
void SrcCefVGUIPanel::MarkTextureDirty( int dirtyx, int dirtyy, int dirtyxend, int dirtyyend )
{
	m_iDirtyX = Min( m_iDirtyX, dirtyx );
	m_iDirtyY = Min( m_iDirtyY, dirtyy );
	m_iDirtyXEnd = Max( m_iDirtyXEnd, dirtyxend );
	m_iDirtyYEnd = Max( m_iDirtyYEnd, dirtyyend );

	if( !m_pTextureRegen )
		return;
	m_pTextureRegen->MakeDirty();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefVGUIPanel::OnSizeChanged(int newWide, int newTall)
{
	BaseClass::OnSizeChanged( newWide, newTall );

	//if( m_pTextureRegen )
	{
		int iWide, iTall;
		GetSize( iWide, iTall );

		m_bSizeChanged = true;
		if( !ResizeTexture( iWide, iTall ) )
			m_bSizeChanged = false;
	}
	/*else
	{
		m_bSizeChanged = true;
	}*/
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefVGUIPanel::PerformLayout()
{
	BaseClass::PerformLayout();
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
void SrcCefVGUIPanel::OnKeyCodePressed(vgui::KeyCode code)
{
	BaseClass::OnKeyCodePressed( code );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefVGUIPanel::OnKeyCodeTyped(vgui::KeyCode code)
{
	BaseClass::OnKeyCodeTyped( code );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefVGUIPanel::OnKeyTyped(wchar_t unichar)
{
	BaseClass::OnKeyTyped( unichar );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefVGUIPanel::OnKeyCodeReleased(vgui::KeyCode code)
{
	BaseClass::OnKeyCodeReleased( code );
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

#if 0
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefVGUIPanel::InternalFocusChanged(bool lost)
{
	if( g_debug_cef.GetBool() )
		DevMsg("CEF: InternalFocusChanged lost:%d\n", lost);
	if( lost )
		m_pBrowser->Unfocus();
	else
		m_pBrowser->Focus();
}
#endif // 0

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	SrcCefVGUIPanel::GetBrowserID()
{
	return m_pBrowser->GetBrowser() ? m_pBrowser->GetBrowser()->GetIdentifier() : -1;
}
