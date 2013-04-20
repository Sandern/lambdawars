//====== Copyright © 2013 Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef SURFACEPASSTRU_H
#define SURFACEPASSTRU_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/ISurface.h>
#include "tier1/utlvector.h"

#include "hl2wars/teamcolor_proxy.h"

#undef CreateFont

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Wraps contextless windows system functions
//-----------------------------------------------------------------------------
class CSurfacePassThru : public ISurface
{
public:
	CSurfacePassThru()
	{
		m_pSurfacePassThru = NULL;
	}
	virtual void InitPassThru( ISurface *pSurfacePassThru )
	{
		m_pSurfacePassThru = pSurfacePassThru;
	}
	ISurface *GetRealSurface() { return m_pSurfacePassThru; }

	// Shouldn't used
	bool Connect( CreateInterfaceFn factory ) { return m_pSurfacePassThru->Connect( factory ); }
	void Disconnect() { m_pSurfacePassThru->Disconnect(); }
	void *QueryInterface( const char *pInterfaceName ) { return m_pSurfacePassThru->QueryInterface( pInterfaceName ); }
	InitReturnVal_t Init() { return m_pSurfacePassThru->Init(); }
	void Shutdown() { m_pSurfacePassThru->Shutdown(); }


	// Returns all dependent libraries
	const AppSystemInfo_t* GetDependencies() { return m_pSurfacePassThru->GetDependencies(); }
	AppSystemTier_t GetTier() { return m_pSurfacePassThru->GetTier(); }
	void Reconnect( CreateInterfaceFn factory, const char *pInterfaceName ) { m_pSurfacePassThru->Reconnect( factory, pInterfaceName ); }

	void RunFrame() { m_pSurfacePassThru->RunFrame(); }
	VPANEL GetEmbeddedPanel();
	void SetEmbeddedPanel( VPANEL pPanel );
	void PushMakeCurrent(VPANEL panel, bool useInsets);
	void PopMakeCurrent(VPANEL panel);

	// rendering functions
	// Will be buffered
	void DrawSetColor(int r, int g, int b, int a);
	void DrawSetColor(Color col);
	
	void DrawFilledRect(int x0, int y0, int x1, int y1);
	void DrawFilledRectArray( IntRect *pRects, int numRects );
	void DrawOutlinedRect(int x0, int y0, int x1, int y1);

	void DrawLine(int x0, int y0, int x1, int y1);
	void DrawPolyLine(int *px, int *py, int numPoints);

	void DrawSetTextFont(HFont font);
	void DrawSetTextColor(int r, int g, int b, int a);
	void DrawSetTextColor(Color col);
	void DrawSetTextPos(int x, int y);
	void DrawGetTextPos(int& x,int& y);
	void DrawPrintText(const wchar_t *text, int textLen, FontDrawType_t drawType = FONT_DRAW_DEFAULT );
	void DrawUnicodeChar(wchar_t wch, FontDrawType_t drawType = FONT_DRAW_DEFAULT );

	void DrawFlushText();		// flushes any buffered text (for rendering optimizations)
	IHTML *CreateHTMLWindow(vgui::IHTMLEvents *events,VPANEL context);
	void PaintHTMLWindow(vgui::IHTML *htmlwin);
	void DeleteHTMLWindow(IHTML *htmlwin);

	int	 DrawGetTextureId( char const *filename );
	bool DrawGetTextureFile(int id, char *filename, int maxlen );
	void DrawSetTextureFile(int id, const char *filename, int hardwareFilter, bool forceReload);
	void DrawSetTextureRGBA(int id, const unsigned char *rgba, int wide, int tall);
	void DrawSetTexture(int id);
	void DrawGetTextureSize(int id, int &wide, int &tall);
	void DrawTexturedRect(int x0, int y0, int x1, int y1);
	bool IsTextureIDValid(int id);

	int CreateNewTextureID( bool procedural = false );
#ifdef _X360
	void DestroyTextureID( int id );
	void UncacheUnusedMaterials();
#endif

	void GetScreenSize(int &wide, int &tall);
	void SetAsTopMost(VPANEL panel, bool state);
	void BringToFront(VPANEL panel);
	void SetForegroundWindow (VPANEL panel);
	void SetPanelVisible(VPANEL panel, bool state);
	void SetMinimized(VPANEL panel, bool state);
	bool IsMinimized(VPANEL panel);
	void FlashWindow(VPANEL panel, bool state);
	void SetTitle(VPANEL panel, const wchar_t *title);
	void SetAsToolBar(VPANEL panel, bool state);		// removes the window's task bar entry (for context menu's, etc.)

	// windows stuff
	void CreatePopup(VPANEL panel, bool minimised, bool showTaskbarIcon = true, bool disabled = false, bool mouseInput = true , bool kbInput = true);
	void SwapBuffers(VPANEL panel);
	void Invalidate(VPANEL panel);
	void SetCursor(HCursor cursor);
	bool IsCursorVisible();
	void ApplyChanges();
	bool IsWithin(int x, int y);
	bool HasFocus();
	
	bool SupportsFeature(SurfaceFeature_t feature);

	// restricts what gets drawn to one panel and it's children
	// currently only works in the game
	void RestrictPaintToSinglePanel(VPANEL panel, bool bForceAllowNonModalSurface = false);

	// these two functions obselete, use IInput::SetAppModalSurface() instead
	void SetModalPanel(VPANEL );
	VPANEL GetModalPanel();

	void UnlockCursor();
	void LockCursor();
	void SetTranslateExtendedKeys(bool state);
	VPANEL GetTopmostPopup();

	// engine-only focus handling (replacing WM_FOCUS windows handling)
	void SetTopLevelFocus(VPANEL panel);

	// fonts
	// creates an empty handle to a vgui font.  windows fonts can be add to this via SetFontGlyphSet().
	HFont CreateFont();

	// adds to the font
	enum EFontFlags
	{
		FONTFLAG_NONE,
		FONTFLAG_ITALIC			= 0x001,
		FONTFLAG_UNDERLINE		= 0x002,
		FONTFLAG_STRIKEOUT		= 0x004,
		FONTFLAG_SYMBOL			= 0x008,
		FONTFLAG_ANTIALIAS		= 0x010,
		FONTFLAG_GAUSSIANBLUR	= 0x020,
		FONTFLAG_ROTARY			= 0x040,
		FONTFLAG_DROPSHADOW		= 0x080,
		FONTFLAG_ADDITIVE		= 0x100,
		FONTFLAG_OUTLINE		= 0x200,
		FONTFLAG_CUSTOM			= 0x400,		// custom generated font - never fall back to asian compatibility mode
		FONTFLAG_BITMAP			= 0x800,		// compiled bitmap font - no fallbacks
	};

	bool SetFontGlyphSet(HFont font, const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags, int nRangeMin = 0, int nRangeMax = 0);

	// adds a custom font file (only supports true type font files (.ttf) for now)
	bool AddCustomFontFile(const char *fontFileName);

	// returns the details about the font
	int GetFontTall(HFont font);
	int GetFontAscent(HFont font, wchar_t wch);
	bool IsFontAdditive(HFont font);
	void GetCharABCwide(HFont font, int ch, int &a, int &b, int &c);
	int GetCharacterWidth(HFont font, int ch);
	void GetTextSize(HFont font, const wchar_t *text, int &wide, int &tall);

	// notify icons?!?
	VPANEL GetNotifyPanel();
	void SetNotifyIcon(VPANEL context, HTexture icon, VPANEL panelToReceiveMessages, const char *text);

	// plays a sound
	void PlaySound(const char *fileName);

	//!! these functions should not be accessed directly, but only through other vgui items
	//!! need to move these to seperate interface
	int GetPopupCount();
	VPANEL GetPopup(int index);
	bool ShouldPaintChildPanel(VPANEL childPanel);
	bool RecreateContext(VPANEL panel);
	void AddPanel(VPANEL panel);
	void ReleasePanel(VPANEL panel);
	void MovePopupToFront(VPANEL panel);
	void MovePopupToBack(VPANEL panel);

	void SolveTraverse(VPANEL panel, bool forceApplySchemeSettings = false);
	void PaintTraverse(VPANEL panel);

	void EnableMouseCapture(VPANEL panel, bool state);

	// returns the size of the workspace
	void GetWorkspaceBounds(int &x, int &y, int &wide, int &tall);

	// gets the absolute coordinates of the screen (in windows space)
	void GetAbsoluteWindowBounds(int &x, int &y, int &wide, int &tall);

	// gets the base resolution used in proportional mode
	void GetProportionalBase( int &width, int &height );

	void CalculateMouseVisible();
	bool NeedKBInput();

	bool HasCursorPosFunctions();
	void SurfaceGetCursorPos(int &x, int &y);
	void SurfaceSetCursorPos(int x, int y);


	// SRC only functions!!!
	void DrawTexturedLine( const Vertex_t &a, const Vertex_t &b );
	void DrawOutlinedCircle(int x, int y, int radius, int segments);
	void DrawTexturedPolyLine( const Vertex_t *p,int n ); // (Note: this connects the first and last points).
	void DrawTexturedSubRect( int x0, int y0, int x1, int y1, float texs0, float text0, float texs1, float text1 );
	void DrawTexturedPolygon(int n, Vertex_t *pVertices, bool bClipVertices = true);
	const wchar_t *GetTitle(VPANEL panel);
	bool IsCursorLocked( void ) const;
	void SetWorkspaceInsets( int left, int top, int right, int bottom );

	// squarish comic book word bubble with pointer, rect params specify the space inside the bubble
	void DrawWordBubble( int x0, int y0, int x1, int y1, int nBorderThickness, Color rgbaBackground, Color rgbaBorder, 
								 bool bPointer = false, int nPointerX = 0, int nPointerY = 0, int nPointerBaseThickness = 16 );


	// Lower level char drawing code, call DrawGet then pass in info to DrawRender
	bool DrawGetUnicodeCharRenderInfo( wchar_t ch, FontCharRenderInfo& info );
	void DrawRenderCharFromInfo( const FontCharRenderInfo& info );

	// global alpha setting functions
	// affect all subsequent draw calls - shouldn't normally be used directly, only in Panel::PaintTraverse()
	void DrawSetAlphaMultiplier( float alpha /* [0..1] */ );
	float DrawGetAlphaMultiplier();

	// web browser
	void SetAllowHTMLJavaScript( bool state );

	// video mode changing
	void OnScreenSizeChanged( int nOldWidth, int nOldHeight );

	vgui::HCursor CreateCursorFromFile( char const *curOrAniFile, char const *pPathID = 0 );

	// create IVguiMatInfo object ( IMaterial wrapper in VguiMatSurface, NULL in CWin32Surface )
	IVguiMatInfo *DrawGetTextureMatInfoFactory( int id );

	void PaintTraverseEx(VPANEL panel, bool paintPopups = false );

	float GetZPos() const;

	// From the Xbox
	void SetPanelForInput( VPANEL vpanel );
	void DrawFilledRectFastFade( int x0, int y0, int x1, int y1, int fadeStartPt, int fadeEndPt, unsigned int alpha0, unsigned int alpha1, bool bHorizontal ) {}
	void DrawFilledRectFade( int x0, int y0, int x1, int y1, unsigned int alpha0, unsigned int alpha1, bool bHorizontal );
	void DrawSetTextureRGBAEx(int id, const unsigned char *rgba, int wide, int tall, ImageFormat imageFormat );
	void DrawSetTextScale(float sx, float sy);
	bool SetBitmapFontGlyphSet(HFont font, const char *windowsFontName, float scalex, float scaley, int flags);
	// adds a bitmap font file
	bool AddBitmapFontFile(const char *fontFileName);
	// sets a symbol for the bitmap font
	void SetBitmapFontName( const char *pName, const char *pFontFilename );
	// gets the bitmap font filename
	const char *GetBitmapFontName( const char *pName );
	void ClearTemporaryFontCache( void );

	IImage *GetIconImageForFullPath( char const *pFullPath );
	void DrawUnicodeString( const wchar_t *pwString, FontDrawType_t drawType = FONT_DRAW_DEFAULT );
	void PrecacheFontCharacters(HFont font, wchar_t *pCharacters);
	// Console-only.  Get the string to use for the current video mode for layout files.
	const char *GetResolutionKey( void ) const;

	// ASW Additions
	const char *GetFontName( HFont font );

	bool ForceScreenSizeOverride( bool bState, int wide, int tall );
	// LocalToScreen, ParentLocalToScreen fixups for explicit PaintTraverse calls on Panels not at 0, 0 position
	bool ForceScreenPosOffset( bool bState, int x, int y );
	void OffsetAbsPos( int &x, int &y );

	void SetAbsPosForContext( int id, int x, int y );
	void GetAbsPosForContext( int id, int &x, int& y );

	// Causes fonts to get reloaded, etc.
	void ResetFontCaches();

	bool IsScreenSizeOverrideActive( void );
	bool IsScreenPosOverrideActive( void );

	void DestroyTextureID( int id );

	int GetTextureNumFrames( int id );
	void DrawSetTextureFrame( int id, int nFrame, unsigned int *pFrameCache );

	void GetClipRect( int &x0, int &y0, int &x1, int &y1 );
	void SetClipRect( int x0, int y0, int x1, int y1 );

	void DrawTexturedRectEx( DrawTexturedRectParms_t *pDrawParms );

	void SetProxyUITeamColor( const Vector &vTeamColor );
	void ClearProxyUITeamColor( void );

protected:
	ISurface *m_pSurfacePassThru;
};

// Functions
inline VPANEL CSurfacePassThru::GetEmbeddedPanel() 
{ 
	return m_pSurfacePassThru->GetEmbeddedPanel(); 
}

inline void CSurfacePassThru::SetEmbeddedPanel( VPANEL pPanel ) 
{ 
	m_pSurfacePassThru->SetEmbeddedPanel( pPanel ); 
}

inline void CSurfacePassThru::PushMakeCurrent(VPANEL panel, bool useInsets) 
{ 
	m_pSurfacePassThru->PushMakeCurrent( panel, useInsets ); 
}

inline void CSurfacePassThru::PopMakeCurrent(VPANEL panel) 
{ 
	m_pSurfacePassThru->PopMakeCurrent( panel );
}

// rendering functions
inline void CSurfacePassThru::DrawSetColor(int r, int g, int b, int a) 
{ 
	// BUFFER CALL
	m_pSurfacePassThru->DrawSetColor(r, g, b, a); 
}
inline void CSurfacePassThru::DrawSetColor(Color col) 
{ 
	// BUFFER CALL
	m_pSurfacePassThru->DrawSetColor(col); 
}

inline void CSurfacePassThru::DrawFilledRect(int x0, int y0, int x1, int y1)
{ 
	// BUFFER CALL
	m_pSurfacePassThru->DrawFilledRect(x0, y0, x1, y1); 
}

inline void CSurfacePassThru::DrawFilledRectArray( IntRect *pRects, int numRects ) 
{ 
	m_pSurfacePassThru->DrawFilledRectArray( pRects, numRects); 
}
inline void CSurfacePassThru::DrawOutlinedRect(int x0, int y0, int x1, int y1) 
{ 
	m_pSurfacePassThru->DrawOutlinedRect(x0, y0, x1, y1); 
}

inline void CSurfacePassThru::DrawLine(int x0, int y0, int x1, int y1) 
{
	m_pSurfacePassThru->DrawLine(x0, y0, x1, y1); 
}

inline void CSurfacePassThru::DrawPolyLine(int *px, int *py, int numPoints) 
{ 
	m_pSurfacePassThru->DrawPolyLine(px, py, numPoints);
}

inline void CSurfacePassThru::DrawSetTextFont(HFont font) 
{ 
	m_pSurfacePassThru->DrawSetTextFont(font); 
}

inline void CSurfacePassThru::DrawSetTextColor(int r, int g, int b, int a) 
{
	m_pSurfacePassThru->DrawSetTextColor(r, g, b, a); 
}

inline void CSurfacePassThru::DrawSetTextColor(Color col) 
{
	m_pSurfacePassThru->DrawSetTextColor(col); 
}

inline void CSurfacePassThru::DrawSetTextPos(int x, int y) 
{
	m_pSurfacePassThru->DrawSetTextPos(x, y); 
}

inline void CSurfacePassThru::DrawGetTextPos(int& x,int& y) 
{
	m_pSurfacePassThru->DrawGetTextPos(x, y); 
}
inline void CSurfacePassThru::DrawPrintText(const wchar_t *text, int textLen, FontDrawType_t drawType ) 
{
	m_pSurfacePassThru->DrawPrintText(text, textLen, drawType); 
}

inline void CSurfacePassThru::DrawUnicodeChar(wchar_t wch, FontDrawType_t drawType ) 
{
	m_pSurfacePassThru->DrawUnicodeChar(wch, drawType); 
}

inline void CSurfacePassThru::DrawFlushText() 
{
	m_pSurfacePassThru->DrawFlushText(); 
}		

inline IHTML *CSurfacePassThru::CreateHTMLWindow(vgui::IHTMLEvents *events,VPANEL context) { return m_pSurfacePassThru->CreateHTMLWindow(events, context); }
inline void CSurfacePassThru::PaintHTMLWindow(vgui::IHTML *htmlwin) { m_pSurfacePassThru->PaintHTMLWindow(htmlwin); }
inline void CSurfacePassThru::DeleteHTMLWindow(IHTML *htmlwin) { m_pSurfacePassThru->DeleteHTMLWindow(htmlwin); }

inline int	 CSurfacePassThru::DrawGetTextureId( char const *filename ) { return m_pSurfacePassThru->DrawGetTextureId(filename); }
inline bool CSurfacePassThru::DrawGetTextureFile(int id, char *filename, int maxlen ) { return m_pSurfacePassThru->DrawGetTextureFile(id, filename, maxlen); }
inline void CSurfacePassThru::DrawSetTextureFile(int id, const char *filename, int hardwareFilter, bool forceReload) { m_pSurfacePassThru->DrawSetTextureFile(id, filename, hardwareFilter, forceReload); }

inline void CSurfacePassThru::DrawSetTextureRGBA(int id, const unsigned char *rgba, int wide, int tall) { m_pSurfacePassThru->DrawSetTextureRGBA(id, rgba, wide, tall); }

inline void CSurfacePassThru::DrawSetTexture(int id) 
{ 
	m_pSurfacePassThru->DrawSetTexture(id);
}
inline void CSurfacePassThru::DrawGetTextureSize(int id, int &wide, int &tall) 
{ 
	m_pSurfacePassThru->DrawGetTextureSize(id, wide, tall); 
}

inline void CSurfacePassThru::DrawTexturedRect(int x0, int y0, int x1, int y1) 
{ 
	m_pSurfacePassThru->DrawTexturedRect(x0, y0, x1, y1); 
}
inline bool CSurfacePassThru::IsTextureIDValid(int id) { return m_pSurfacePassThru->IsTextureIDValid(id); }

inline int CSurfacePassThru::CreateNewTextureID( bool procedural ) { return m_pSurfacePassThru->CreateNewTextureID(procedural); }

inline void CSurfacePassThru::GetScreenSize(int &wide, int &tall) { m_pSurfacePassThru->GetScreenSize(wide, tall); }
inline void CSurfacePassThru::SetAsTopMost(VPANEL panel, bool state) { m_pSurfacePassThru->SetAsTopMost(panel, state); }
inline void CSurfacePassThru::BringToFront(VPANEL panel) { m_pSurfacePassThru->BringToFront(panel); }
inline void CSurfacePassThru::SetForegroundWindow (VPANEL panel) { m_pSurfacePassThru->SetForegroundWindow(panel); }
inline void CSurfacePassThru::SetPanelVisible(VPANEL panel, bool state) 
{ 
	m_pSurfacePassThru->SetPanelVisible(panel, state); 
}

inline void CSurfacePassThru::SetMinimized(VPANEL panel, bool state) { m_pSurfacePassThru->SetMinimized(panel, state); }
inline bool CSurfacePassThru::IsMinimized(VPANEL panel) { return m_pSurfacePassThru->IsMinimized(panel); }
inline void CSurfacePassThru::FlashWindow(VPANEL panel, bool state) { m_pSurfacePassThru->FlashWindow(panel, state); }
inline void CSurfacePassThru::SetTitle(VPANEL panel, const wchar_t *title) { m_pSurfacePassThru->SetTitle(panel, title); }
inline void CSurfacePassThru::SetAsToolBar(VPANEL panel, bool state) { m_pSurfacePassThru->SetAsToolBar(panel, state); }

// windows stuff
inline void CSurfacePassThru::CreatePopup(VPANEL panel, bool minimised, bool showTaskbarIcon, bool disabled, bool mouseInput , bool kbInput)
                     { m_pSurfacePassThru->CreatePopup(panel, minimised, showTaskbarIcon, disabled, mouseInput, kbInput); }
inline void CSurfacePassThru::SwapBuffers(VPANEL panel) { m_pSurfacePassThru->SwapBuffers(panel); }
inline void CSurfacePassThru::Invalidate(VPANEL panel) 
{ 
	m_pSurfacePassThru->Invalidate(panel);
}
inline void CSurfacePassThru::SetCursor(HCursor cursor) { m_pSurfacePassThru->SetCursor(cursor); }
inline bool CSurfacePassThru::IsCursorVisible() { return m_pSurfacePassThru->IsCursorVisible(); }
inline void CSurfacePassThru::ApplyChanges() { m_pSurfacePassThru->ApplyChanges(); }
inline bool CSurfacePassThru::IsWithin(int x, int y) { return m_pSurfacePassThru->IsWithin(x, y); }
inline bool CSurfacePassThru::HasFocus() { return m_pSurfacePassThru->HasFocus(); }

// returns true if the surface supports minimize & maximize capabilities
inline bool CSurfacePassThru::SupportsFeature(ISurface::SurfaceFeature_t feature) { return m_pSurfacePassThru->SupportsFeature(feature); }

// restricts what gets drawn to one panel and it's children
// currently only works in the game
inline void CSurfacePassThru::RestrictPaintToSinglePanel(VPANEL panel, bool bForceAllowNonModalSurface) { m_pSurfacePassThru->RestrictPaintToSinglePanel(panel, bForceAllowNonModalSurface); }

// these two functions obselete, use IInput::SetAppModalSurface() instead
inline void CSurfacePassThru::SetModalPanel(VPANEL panel ) { m_pSurfacePassThru->SetModalPanel(panel); }
inline VPANEL CSurfacePassThru::GetModalPanel() { return m_pSurfacePassThru->GetModalPanel(); }

inline void CSurfacePassThru::UnlockCursor() { m_pSurfacePassThru->UnlockCursor(); }
inline void CSurfacePassThru::LockCursor() { m_pSurfacePassThru->LockCursor(); }
inline void CSurfacePassThru::SetTranslateExtendedKeys(bool state) { m_pSurfacePassThru->SetTranslateExtendedKeys(state); }
inline VPANEL CSurfacePassThru::GetTopmostPopup() { return m_pSurfacePassThru->GetTopmostPopup(); }

// engine-only focus handling (replacing WM_FOCUS windows handling)
inline void CSurfacePassThru::SetTopLevelFocus(VPANEL panel) { m_pSurfacePassThru->SetTopLevelFocus(panel); }

// fonts
// creates an empty handle to a vgui font.  windows fonts can be add to this via SetFontGlyphSet().
inline HFont CSurfacePassThru::CreateFont() { return m_pSurfacePassThru->CreateFont(); }

inline bool CSurfacePassThru::SetFontGlyphSet(HFont font, const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags, int nRangeMin, int nRangeMax)
            { return m_pSurfacePassThru->SetFontGlyphSet(font, windowsFontName, tall, weight, blur, scanlines, flags, nRangeMin, nRangeMax); }

// adds a custom font file (only supports true type font files (.ttf) for now)
inline bool CSurfacePassThru::AddCustomFontFile(const char *fontFileName) { return m_pSurfacePassThru->AddCustomFontFile(fontFileName); }

// returns the details about the font
inline int CSurfacePassThru::GetFontTall(HFont font) { return m_pSurfacePassThru->GetFontTall(font); }
inline int CSurfacePassThru::GetFontAscent(HFont font, wchar_t wch) { return m_pSurfacePassThru->GetFontAscent(font, wch); }
inline bool CSurfacePassThru::IsFontAdditive(HFont font) { return m_pSurfacePassThru->IsFontAdditive(font); }
inline void CSurfacePassThru::GetCharABCwide(HFont font, int ch, int &a, int &b, int &c) { m_pSurfacePassThru->GetCharABCwide(font, ch, a, b, c); }
inline int CSurfacePassThru::GetCharacterWidth(HFont font, int ch) { return m_pSurfacePassThru->GetCharacterWidth(font, ch); }
inline void CSurfacePassThru::GetTextSize(HFont font, const wchar_t *text, int &wide, int &tall) { m_pSurfacePassThru->GetTextSize(font, text, wide, tall); }

// notify icons?!?
inline VPANEL CSurfacePassThru::GetNotifyPanel() { return m_pSurfacePassThru->GetNotifyPanel(); }
inline void CSurfacePassThru::SetNotifyIcon(VPANEL context, HTexture icon, VPANEL panelToReceiveMessages, const char *text) 
        { m_pSurfacePassThru->SetNotifyIcon(context, icon, panelToReceiveMessages, text); }

// plays a sound
inline void CSurfacePassThru::PlaySound(const char *fileName) { m_pSurfacePassThru->PlaySound(fileName); }

//!! these functions should not be accessed directly, but only through other vgui items
//!! need to move these to seperate interface
inline int CSurfacePassThru::GetPopupCount() { return m_pSurfacePassThru->GetPopupCount(); }
inline VPANEL CSurfacePassThru::GetPopup(int index) { return m_pSurfacePassThru->GetPopup(index); }
inline bool CSurfacePassThru::ShouldPaintChildPanel(VPANEL childPanel) { return m_pSurfacePassThru->ShouldPaintChildPanel(childPanel); }
inline bool CSurfacePassThru::RecreateContext(VPANEL panel) { return m_pSurfacePassThru->RecreateContext(panel); }
inline void CSurfacePassThru::AddPanel(VPANEL panel) { m_pSurfacePassThru->AddPanel(panel); }
inline void CSurfacePassThru::ReleasePanel(VPANEL panel) { m_pSurfacePassThru->ReleasePanel(panel); }
inline void CSurfacePassThru::MovePopupToFront(VPANEL panel) { m_pSurfacePassThru->MovePopupToFront(panel); }
inline void CSurfacePassThru::MovePopupToBack(VPANEL panel) { m_pSurfacePassThru->MovePopupToBack(panel); }

inline void CSurfacePassThru::SolveTraverse(VPANEL panel, bool forceApplySchemeSettings) 
{ 
	m_pSurfacePassThru->SolveTraverse(panel, forceApplySchemeSettings); 
}

inline void CSurfacePassThru::PaintTraverse(VPANEL panel) 
{ 
	m_pSurfacePassThru->PaintTraverse(panel); 
}

inline void CSurfacePassThru::EnableMouseCapture(VPANEL panel, bool state) { m_pSurfacePassThru->EnableMouseCapture(panel, state); }

// returns the size of the workspace
inline void CSurfacePassThru::GetWorkspaceBounds(int &x, int &y, int &wide, int &tall) { m_pSurfacePassThru->GetWorkspaceBounds(x, y, wide, tall); }

// gets the absolute coordinates of the screen (in windows space)
inline void CSurfacePassThru::GetAbsoluteWindowBounds(int &x, int &y, int &wide, int &tall) { m_pSurfacePassThru->GetAbsoluteWindowBounds(x, y, wide, tall); }

// gets the base resolution used in proportional mode
inline void CSurfacePassThru::GetProportionalBase( int &width, int &height ) { m_pSurfacePassThru->GetProportionalBase(width, height); }

inline void CSurfacePassThru::CalculateMouseVisible() { m_pSurfacePassThru->CalculateMouseVisible(); }
inline bool CSurfacePassThru::NeedKBInput() { return m_pSurfacePassThru->NeedKBInput(); }

inline bool CSurfacePassThru::HasCursorPosFunctions() { return m_pSurfacePassThru->HasCursorPosFunctions(); }
inline void CSurfacePassThru::SurfaceGetCursorPos(int &x, int &y) { m_pSurfacePassThru->SurfaceGetCursorPos(x, y); }
inline void CSurfacePassThru::SurfaceSetCursorPos(int x, int y) { m_pSurfacePassThru->SurfaceSetCursorPos(x, y); }

inline void CSurfacePassThru::DrawTexturedLine( const Vertex_t &a, const Vertex_t &b ) 
{ 
	m_pSurfacePassThru->DrawTexturedLine(a, b); 

}
inline void CSurfacePassThru::DrawOutlinedCircle(int x, int y, int radius, int segments) 
{ 
	m_pSurfacePassThru->DrawOutlinedCircle(x, y, radius, segments); 
}
inline void CSurfacePassThru::DrawTexturedPolyLine( const Vertex_t *p,int n ) 
{
	m_pSurfacePassThru->DrawTexturedPolyLine( p, n ); 
} 
inline void CSurfacePassThru::DrawTexturedSubRect( int x0, int y0, int x1, int y1, float texs0, float text0, float texs1, float text1 ) 
{
	m_pSurfacePassThru->DrawTexturedSubRect(x0, y0, x1, y1, texs0, text0, texs1, text1); 
}
inline void CSurfacePassThru::DrawTexturedPolygon(int n, Vertex_t *pVertices, bool bClipVertices ) 
{
	m_pSurfacePassThru->DrawTexturedPolygon( n, pVertices, bClipVertices );
}

inline const wchar_t *CSurfacePassThru::GetTitle(VPANEL panel) { return m_pSurfacePassThru->GetTitle(panel); }
inline bool CSurfacePassThru::IsCursorLocked( void ) const { return m_pSurfacePassThru->IsCursorLocked(); }
inline void CSurfacePassThru::SetWorkspaceInsets( int left, int top, int right, int bottom ) { m_pSurfacePassThru->SetWorkspaceInsets(left, top, right, bottom); }

inline void CSurfacePassThru::DrawWordBubble( int x0, int y0, int x1, int y1, int nBorderThickness, Color rgbaBackground, Color rgbaBorder, 
								bool bPointer, int nPointerX, int nPointerY, int nPointerBaseThickness )
{
	m_pSurfacePassThru->DrawWordBubble( x0, y0, x1, y1, nBorderThickness, rgbaBackground, rgbaBorder, 
		bPointer, nPointerX, nPointerY, nPointerBaseThickness );
}

// Lower level char drawing code, call DrawGet then pass in info to DrawRender
inline bool CSurfacePassThru::DrawGetUnicodeCharRenderInfo( wchar_t ch, FontCharRenderInfo& info ) { return m_pSurfacePassThru->DrawGetUnicodeCharRenderInfo(ch, info); }
inline void CSurfacePassThru::DrawRenderCharFromInfo( const FontCharRenderInfo& info ) { m_pSurfacePassThru->DrawRenderCharFromInfo(info); }

// global alpha setting functions
// affect all subsequent draw calls - shouldn't normally be used directly, only in Panel::PaintTraverse()
inline void CSurfacePassThru::DrawSetAlphaMultiplier( float alpha /* [0..1] */ ) { m_pSurfacePassThru->DrawSetAlphaMultiplier(alpha); }
inline float CSurfacePassThru::DrawGetAlphaMultiplier() { return m_pSurfacePassThru->DrawGetAlphaMultiplier(); }

// web browser
inline void CSurfacePassThru::SetAllowHTMLJavaScript( bool state ) { m_pSurfacePassThru->SetAllowHTMLJavaScript(state); }

// video mode changing
inline void CSurfacePassThru::OnScreenSizeChanged( int nOldWidth, int nOldHeight ) { m_pSurfacePassThru->OnScreenSizeChanged(nOldWidth, nOldHeight); }

inline vgui::HCursor CSurfacePassThru::CreateCursorFromFile( char const *curOrAniFile, char const *pPathID ) { return m_pSurfacePassThru->CreateCursorFromFile( curOrAniFile, pPathID ); }

// create IVguiMatInfo object ( IMaterial wrapper in VguiMatSurface, NULL in CWin32Surface )
inline IVguiMatInfo *CSurfacePassThru::DrawGetTextureMatInfoFactory( int id ) { return m_pSurfacePassThru->DrawGetTextureMatInfoFactory(id); }

inline void CSurfacePassThru::PaintTraverseEx(VPANEL panel, bool paintPopups ) 
{ 
	m_pSurfacePassThru->PaintTraverseEx(panel, paintPopups); 
}

inline float CSurfacePassThru::GetZPos() const { return m_pSurfacePassThru->GetZPos(); }

// From the Xbox
inline void CSurfacePassThru::SetPanelForInput( VPANEL vpanel ) { m_pSurfacePassThru->SetPanelForInput( vpanel ); }
inline void CSurfacePassThru::DrawFilledRectFade( int x0, int y0, int x1, int y1, unsigned int alpha0, unsigned int alpha1, bool bHorizontal ) 
{ 
	m_pSurfacePassThru->DrawFilledRectFade( x0, y0, x1, y1, alpha0, alpha1, bHorizontal ); 
	
}
inline void CSurfacePassThru::DrawSetTextureRGBAEx(int id, const unsigned char *rgba, int wide, int tall, ImageFormat imageFormat ) 
{ 
	m_pSurfacePassThru->DrawSetTextureRGBAEx( id, rgba, wide, tall, imageFormat ); 
}

inline void CSurfacePassThru::DrawSetTextScale(float sx, float sy) 
{
	m_pSurfacePassThru->DrawSetTextScale( sx, sy );
}
inline bool CSurfacePassThru::SetBitmapFontGlyphSet(HFont font, const char *windowsFontName, float scalex, float scaley, int flags) { return m_pSurfacePassThru->SetBitmapFontGlyphSet( font, windowsFontName, scalex, scaley, flags ); }
// adds a bitmap font file
inline bool CSurfacePassThru::AddBitmapFontFile(const char *fontFileName) { return m_pSurfacePassThru->AddBitmapFontFile(fontFileName); }
// sets a symbol for the bitmap font
inline void CSurfacePassThru::SetBitmapFontName( const char *pName, const char *pFontFilename ) { m_pSurfacePassThru->SetBitmapFontName( pName, pFontFilename ); }
// gets the bitmap font filename
inline const char *CSurfacePassThru::GetBitmapFontName( const char *pName ) { return m_pSurfacePassThru->GetBitmapFontName(pName); }
inline void CSurfacePassThru::ClearTemporaryFontCache( void ) { m_pSurfacePassThru->ClearTemporaryFontCache(); }

inline IImage *CSurfacePassThru::GetIconImageForFullPath( char const *pFullPath ) { return m_pSurfacePassThru->GetIconImageForFullPath(pFullPath); }
inline void CSurfacePassThru::DrawUnicodeString( const wchar_t *pwString, FontDrawType_t drawType )
{ 
	m_pSurfacePassThru->DrawUnicodeString( pwString, drawType ); 
}

inline void CSurfacePassThru::PrecacheFontCharacters(HFont font, wchar_t *pCharacters) { m_pSurfacePassThru->PrecacheFontCharacters(font, pCharacters); }
// Console-only.  Get the string to use for the current video mode for layout files.
inline const char *CSurfacePassThru::GetResolutionKey( void ) const { return m_pSurfacePassThru->GetResolutionKey(); }

// ASW
inline const char *CSurfacePassThru::GetFontName( HFont font ) 
{ 
	return m_pSurfacePassThru->GetFontName( font ); 
}

inline bool CSurfacePassThru::ForceScreenSizeOverride( bool bState, int wide, int tall ) 
{ 
	return m_pSurfacePassThru->ForceScreenSizeOverride( bState, wide, tall );
}

// LocalToScreen, ParentLocalToScreen fixups for explicit PaintTraverse calls on Panels not at 0, 0 position
inline bool CSurfacePassThru::ForceScreenPosOffset( bool bState, int x, int y )  
{ 
	return m_pSurfacePassThru->ForceScreenPosOffset( bState, x, y );;
}

inline void CSurfacePassThru::OffsetAbsPos( int &x, int &y ) 
{
	m_pSurfacePassThru->OffsetAbsPos( x, y );
}

inline void CSurfacePassThru::SetAbsPosForContext( int id, int x, int y ) 
{
	m_pSurfacePassThru->SetAbsPosForContext( id, x, y );
}

inline void CSurfacePassThru::GetAbsPosForContext( int id, int &x, int& y ) 
{
	m_pSurfacePassThru->GetAbsPosForContext( id, x, y );
}

// Causes fonts to get reloaded, etc.
inline void CSurfacePassThru::ResetFontCaches() 
{
	m_pSurfacePassThru->ResetFontCaches();
}

inline bool CSurfacePassThru::IsScreenSizeOverrideActive( void ) 
{ 
	return m_pSurfacePassThru->IsScreenSizeOverrideActive(); 
}

inline bool CSurfacePassThru::IsScreenPosOverrideActive( void )
{ 
	return m_pSurfacePassThru->IsScreenPosOverrideActive(); 
}

inline void CSurfacePassThru::DestroyTextureID( int id ) 
{
	m_pSurfacePassThru->DestroyTextureID( id );
}

inline int CSurfacePassThru::GetTextureNumFrames( int id ) 
{ 
	return m_pSurfacePassThru->GetTextureNumFrames( id );
}

inline void CSurfacePassThru::DrawSetTextureFrame( int id, int nFrame, unsigned int *pFrameCache )
{
	m_pSurfacePassThru->DrawSetTextureFrame( id, nFrame, pFrameCache );
}

inline void CSurfacePassThru::GetClipRect( int &x0, int &y0, int &x1, int &y1 ) 
{
	m_pSurfacePassThru->GetClipRect( x0, y0, x1, y1 );
}
inline void CSurfacePassThru::SetClipRect( int x0, int y0, int x1, int y1 ) 
{
	m_pSurfacePassThru->SetClipRect( x0, y0, x1, y1 );
}

inline void CSurfacePassThru::DrawTexturedRectEx( DrawTexturedRectParms_t *pDrawParms ) 
{
	m_pSurfacePassThru->DrawTexturedRectEx( pDrawParms );
}

inline void CSurfacePassThru::SetProxyUITeamColor( const Vector &vTeamColor )
{
	::SetProxyUITeamColor( vTeamColor );
}

inline void CSurfacePassThru::ClearProxyUITeamColor( void )
{
	::ClearProxyUITeamColor();
}

}

#endif // SURFACEPASSTRU_H