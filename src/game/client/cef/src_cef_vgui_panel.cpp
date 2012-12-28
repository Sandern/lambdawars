//====== Copyright © 20xx, Sander Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "src_cef_vgui_panel.h"
#include "src_cef.h"
#include "src_cef_texgen.h"
#include "clientmode_shared.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

ConVar g_cef_draw("g_cef_draw", "1");

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
SrcCefVGUIPanel::SrcCefVGUIPanel( SrcCefBrowser *pController, vgui::Panel *pParent ) 
	: Panel( NULL, "SrcCefPanel" ), m_pBrowser(pController), m_iTextureID(-1), m_bSizeChanged(false), m_pTextureRegen(NULL)
{
	SetPaintBackgroundEnabled( false );

	SetParent( pParent ? pParent : GetClientMode()->GetViewport() );

	m_Color = Color(255, 255, 255, 255);
	m_iTexWide = m_iTexTall = 0;

	static int staticMatWebViewID = 0;
	Q_snprintf( m_MatWebViewName, _MAX_PATH, "vgui/webview/webview_test%d", staticMatWebViewID++ );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
SrcCefVGUIPanel::~SrcCefVGUIPanel()
{
	m_pBrowser = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefVGUIPanel::ResizeTexture( int width, int height )
{
	m_iWVWide = width;
	m_iWVTall = height;

	int po2wide = nexthigher(m_iWVWide);
	int po2tall = nexthigher(m_iWVTall);

	if( g_debug_cef.GetBool() )
		DevMsg("WebView: Resizing texture to %d %d (wish size: %d %d)\n", po2wide, po2tall, m_iWVWide, m_iWVTall );

	// Only rebuild when the actual size change and don't bother if the size
	// is larger than needed.
	// Otherwise just need to update m_fTexS1 and m_fTexT1 :)
	if( m_iTexWide >= po2wide && m_iTexTall >= po2tall )
	{
		m_fTexS1 = 1.0 - (m_iTexWide - m_iWVWide) / (float)m_iTexWide;
		m_fTexT1 = 1.0 - (m_iTexTall - m_iWVTall) / (float)m_iTexTall;
		return;
	}

	m_iTexWide = po2wide;
	m_iTexTall = po2tall;
	m_fTexS1 = 1.0 - (m_iTexWide - m_iWVWide) / (float)m_iTexWide;
	m_fTexT1 = 1.0 - (m_iTexTall - m_iWVTall) / (float)m_iTexTall;

	if( !m_pTextureRegen )
	{
		if( g_debug_cef.GetBool() )
			DevMsg("WebView: initializing texture regenerator\n");
		m_pTextureRegen = new CCefTextureGenerator( m_pBrowser );
	}
	else
	{
		if( g_debug_cef.GetBool() )
			DevMsg("WebView: Invalidating old render texture\n");
		m_RenderBuffer->SetTextureRegenerator( NULL );
		m_RenderBuffer.Shutdown();
		m_MatRef.Shutdown();
	}

	static int staticTextureID = 0;
	Q_snprintf( m_TextureWebViewName, _MAX_PATH, "_rt_test%d", staticTextureID++ );

	if( g_debug_cef.GetBool() )
		DevMsg("WebView: initializing texture %s\n", m_TextureWebViewName);

	// IMPORTANT: Use TEXTUREFLAGS_POINTSAMPLE. Otherwise mat_filtertextures will make it really blurred and ugly.
	// IMPORTANT 2: Use TEXTUREFLAGS_SINGLECOPY in case you want to be able to regenerate only a part of the texture (i.e. specifiy
	//				a sub rect when calling ->Download()).
	m_RenderBuffer.InitProceduralTexture( m_TextureWebViewName, TEXTURE_GROUP_VGUI, m_iTexWide, m_iTexTall, IMAGE_FORMAT_BGRA8888, 
		TEXTUREFLAGS_PROCEDURAL|TEXTUREFLAGS_NOLOD|TEXTUREFLAGS_NOMIP|TEXTUREFLAGS_POINTSAMPLE|TEXTUREFLAGS_SINGLECOPY );
	if( !m_RenderBuffer.IsValid() )
	{
		Warning("Failed to initialize render buffer texture\n");
		return;
	}

	//m_pTextureRegen->ScheduleInit();
	m_RenderBuffer->SetTextureRegenerator( m_pTextureRegen );
	m_RenderBuffer->Download( NULL );	

	//if( m_MatRef.IsValid() )
	//	m_MatRef.Shutdown();

	if( g_debug_cef.GetBool() )
		DevMsg("WebView: initializing material %s...", m_MatWebViewName);

	// Make sure the directory exists
	if( filesystem->FileExists("materials/vgui/webview", "MOD") == false )
	{
		filesystem->CreateDirHierarchy("materials/vgui/webview", "MOD");
	}

	// Create a material
	char MatBufName[_MAX_PATH];
	Q_snprintf( MatBufName, _MAX_PATH, "materials/%s.vmt", m_MatWebViewName );
	KeyValues *pVMT = new KeyValues("UnlitGeneric");
	pVMT->SetString("$basetexture", m_TextureWebViewName);
	pVMT->SetString("$translucent", "1");
	pVMT->SetString("$no_fullbright", "1");
	pVMT->SetString("$ignorez", "1");

#if 1
	// Save to file
	pVMT->SaveToFile(filesystem, MatBufName, "MOD");

	m_MatRef.Init(m_MatWebViewName, TEXTURE_GROUP_VGUI, true);
	materials->ReloadMaterials(m_MatWebViewName);
#else
	// TODO: Figure out how to use the following instead of writing to file:
	//		 This should init it as a procedural material, but it doesn't seems to work...
	m_MatRef.Init(m_MatWebViewName, TEXTURE_GROUP_VGUI, pVMT);
#endif // 1

	pVMT->deleteThis();

	if( g_debug_cef.GetBool() )
		DevMsg("done.\n");
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
		m_bSizeChanged = false;

		// Update texture
		ResizeTexture( iWide, iTall );

		// Update texture ID
		if( g_debug_cef.GetBool() )
			DevMsg("CEF: initializing texture id...");

		m_iTextureID = vgui::surface()->CreateNewTextureID( true );
		vgui::surface()->DrawSetTextureFile( m_iTextureID, m_MatWebViewName, false, false );

		if( g_debug_cef.GetBool() )
			DevMsg("done.\n");
	}

	if( !m_pTextureRegen )
		return;

	// Draw
	if( m_pTextureRegen->IsDirty() )
	{
		if( g_debug_cef.GetBool() )
			DevMsg("CEF: texture requires full regeneration\n");
		m_RenderBuffer->SetTextureRegenerator( m_pTextureRegen );
		m_RenderBuffer->Download( NULL );	

		if( m_pTextureRegen->IsDirty() == false )
			materials->ReloadMaterials(m_MatWebViewName); // FIXME
	}

	// Must have a valid texture and the texture regenerator should be valid
	if( m_iTextureID != -1 && m_pTextureRegen->IsDirty() == false && g_cef_draw.GetBool() )
	{
		vgui::surface()->DrawSetColor( m_Color );
		vgui::surface()->DrawSetTexture( m_iTextureID );
		vgui::surface()->DrawTexturedSubRect( 0, 0, iWide, iTall, 0, 0, m_fTexS1, m_fTexT1 );
	}

	m_pTextureRegen->MakeDirty();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefVGUIPanel::OnSizeChanged(int newWide, int newTall)
{
	BaseClass::OnSizeChanged( newWide, newTall );

	m_bSizeChanged = true;
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
void SrcCefVGUIPanel::OnCursorMoved( int x,int y )
{
	m_iMouseX = x;
	m_iMouseY = y;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefVGUIPanel::OnMousePressed(vgui::MouseCode code)
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefVGUIPanel::OnMouseDoublePressed(vgui::MouseCode code)
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefVGUIPanel::OnMouseReleased(vgui::MouseCode code)
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefVGUIPanel::OnMouseWheeled( int delta )
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefVGUIPanel::OnKeyCodePressed(vgui::KeyCode code)
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefVGUIPanel::OnKeyCodeTyped(vgui::KeyCode code)
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefVGUIPanel::OnKeyTyped(wchar_t unichar)
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SrcCefVGUIPanel::OnKeyCodeReleased(vgui::KeyCode code)
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
vgui::HCursor SrcCefVGUIPanel::GetCursor()
{
#if 0
	if( !m_pWebView )
	{
		if( GetParent() )
			return GetParent()->GetCursor();
		return BaseClass::GetCursor();
	}

	if( m_pController->GetPassMouseTruIfAlphaZero() && m_pController->IsAlphaZeroAt( m_iMouseX, m_iMouseY ) )
	{
		if( GetParent() )
			return GetParent()->GetCursor();
	}
#endif // 0

	return BaseClass::GetCursor();
}