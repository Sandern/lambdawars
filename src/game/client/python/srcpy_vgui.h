//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: Hax for asw vgui python and buffered paint calls
//
// $NoKeywords: $
//=============================================================================//

#ifndef SRCPY_VGUI_H
#define SRCPY_VGUI_H

#ifdef _WIN32
#pragma once
#endif
#include <vgui/IVGui.h>
#include <vgui_controls/Controls.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <ienginevgui.h>
#include "vgui_controls/Panel.h"

#include "SurfaceBuffer.h"

#include "srcpy_boostpython.h"

extern const char *vgui::GetControlsModuleName();

using namespace vgui;

extern int g_PythonPanelCount;

//=============================================================================
// Message handling. Can't be merged in Panel, since we do not have the source code :(
//=============================================================================
typedef struct py_message_entry_t {
	int numParams;
	boost::python::object method;

	const char *firstParamName;
	int firstParamSymbol;
	int firstParamType;

	const char *secondParamName;
	int secondParamSymbol;
	int secondParamType;
} py_message_entry_t;
bool Panel_DispatchMessage(CUtlDict<py_message_entry_t, short> &messageMap, const KeyValues *params, VPANEL fromPanel);

//-----------------------------------------------------------------------------
// Purpose: PyPanel
//-----------------------------------------------------------------------------
class PyPanel
{
public:
	friend class BufferCallPaintTraverse;

	typedef struct CallBuffer_t {
		bool m_bIsRecording;
		CUtlVector< CUtlVector< BaseBufferCall * > > m_SCallBuffer;
		int m_iNumSBufferCall;
		double m_SurfaceCallRecordFrameTime;
		double m_SurfaceCallDrawFrameTime;
		bool m_bShouldRecordBuffer;
	} CallBuffer_t;

	PyPanel();
	~PyPanel();

	virtual Panel *GetPanel() { return NULL; }
	virtual PyObject *GetPySelf() const = 0;

public:
    /*void RegMessageMethod( const char *message, boost::python::object method, int numParams=0, 
           const char *nameFirstParam="", int typeFirstParam=DATATYPE_VOID, 
           const char *nameSecondParam="", int typeSecondParam=DATATYPE_VOID ) { 
           py_message_entry_t entry;
           entry.method = method;
           entry.numParams = numParams;
           entry.firstParamName = nameFirstParam;
           entry.firstParamSymbol = KeyValuesSystem()->GetSymbolForString(nameFirstParam);
           entry.firstParamType = typeFirstParam;
           entry.secondParamName = nameSecondParam;
           entry.secondParamSymbol = KeyValuesSystem()->GetSymbolForString(nameSecondParam);
           entry.secondParamType = typeSecondParam;
    
           m_PyMessageMap.Insert(message, entry);
    }*/

	CUtlDict<py_message_entry_t, short> &GetPyMessageMap() { return m_PyMessageMap; }
	void ClearPyMessageMap() { m_PyMessageMap.Purge(); }

public:
	void ClearSBuffer( CallBuffer_t &CallBuffer );
	void ClearAllSBuffers();

	void EnableSBuffer( bool bUseBuffer ) { m_bUseSurfaceCallBuffer = bUseBuffer; }
	bool IsSBufferEnabled( void );
	void FlushSBuffer( void );
	void ParentFlushSBuffer();
	void SetFlushedByParent( bool bEnabled ) { m_bFlushedByParent = bEnabled; }
	bool IsFlushedByParent() { return m_bFlushedByParent; }

	bool ShouldRecordSBuffer( CallBuffer_t &CallBuffer );
	void FinishRecordSBuffer(CallBuffer_t &CallBuffer );
	bool IsRecordingSBuffer();
	void DrawFromSBuffer( CallBuffer_t &CallBuffer );

	bool IsPyDeleted() { return m_bPyDeleted; }
	void SetPyDeleted() { m_bPyDeleted = true; }

private:
	bool m_bUseSurfaceCallBuffer;
	bool m_bFlushedByParent;

public:
	bool m_bPyDeleted;

protected:
	CUtlDict<py_message_entry_t, short> m_PyMessageMap;

	//CallBuffer_t m_PaintBorderCallBuffer;
	CallBuffer_t m_PaintCallBuffer;
	CallBuffer_t m_PaintBackgroundCallBuffer;
};

inline void PyPanel::ClearAllSBuffers()
{
	ClearSBuffer( m_PaintCallBuffer );
	ClearSBuffer( m_PaintBackgroundCallBuffer );
}

void DestroyPyPanels();

//-----------------------------------------------------------------------------
// Purpose: Base Panel handle
//-----------------------------------------------------------------------------
class PyBaseVGUIHandle
{
public:
	PyBaseVGUIHandle() : m_hPanelHandle(INVALID_PANEL) {}

	PyBaseVGUIHandle(VPANEL handle)
	{
		m_hPanelHandle = ivgui()->PanelToHandle( handle );
	}

	// Assign a value to the handle.
	Panel *Get() const;

	// Set
	const PyBaseVGUIHandle& Set( IClientPanel *pPanel );

protected:
	HPanel m_hPanelHandle;
};

//-----------------------------------------------------------------------------
// Purpose: Panel handle
//-----------------------------------------------------------------------------
template< class T >
class PyVGUIHandle : public PyBaseVGUIHandle
{
public:
	PyVGUIHandle() {}

	PyVGUIHandle(T *panel)
	{
		Set(panel);
	}
	PyVGUIHandle(VPANEL handle)
	{
		m_hPanelHandle = ivgui()->PanelToHandle(handle);
	}

	boost::python::object GetAttr( const char *name )
	{
		return boost::python::object(boost::python::ptr(Get())).attr(name);
	}

	T *Get();
	void Set( T *pPanel );

	operator T *()							{ return Get(); }
	T * operator ->()						{ return Get(); }
	T * operator = (T *pPanel)				{ return Set(pPanel); }

	bool operator == (T *pPanel)			{ return (Get() == pPanel); }
	operator bool ()						{ return Get() != 0; }
};


//-----------------------------------------------------------------------------
// Purpose: Panel handle implementation
//			Returns a pointer to a valid panel, NULL if the panel has been deleted
//-----------------------------------------------------------------------------
template< class T >
T *PyVGUIHandle<T>::Get() 
{
	if (m_hPanelHandle != INVALID_PANEL)
	{
		VPANEL panel = ivgui()->HandleToPanel(m_hPanelHandle);
		if (panel)
		{
			T *vguiPanel = dynamic_cast<T *>(ipanel()->GetPanel(panel, GetControlsModuleName()));
			return vguiPanel;
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: sets the smart pointer
//-----------------------------------------------------------------------------
template< class T >
void PyVGUIHandle<T>::Set(T *pent)
{
	if (pent)
	{
		m_hPanelHandle = ivgui()->PanelToHandle(pent->GetVPanel());
	}
	else
	{
		m_hPanelHandle = INVALID_PANEL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Destroyed python panel.
//-----------------------------------------------------------------------------
class DeadPanel 
{
public:
	static bool NonZero() { return false; }
};

PyObject *GetPyPanel( Panel *pPanel );
void PyDeletePanel( Panel *pPanel, PyPanel *pPyPanel, int iRemoveIdx = -1 );

//=============================================================================
// ISurface for Python
//=============================================================================
class CWrapSurface
{
public:
	// hierarchy root
	inline VPANEL GetEmbeddedPanel() { return surface()->GetEmbeddedPanel(); }
	inline void SetEmbeddedPanel( VPANEL pPanel ) { surface()->SetEmbeddedPanel(pPanel); }

	// drawing context
	inline void PushMakeCurrent(VPANEL panel, bool useInsets) { surface()->PushMakeCurrent(panel, useInsets); }
	inline void PopMakeCurrent(VPANEL panel) { surface()->PopMakeCurrent(panel); }

	// rendering functions
	inline void DrawSetColor(int r, int g, int b, int a) { surface()->DrawSetColor(r, g, b, a); }
	inline void DrawSetColor(Color col) { surface()->DrawSetColor(col); }

	inline void DrawFilledRect(int x0, int y0, int x1, int y1) { surface()->DrawFilledRect(x0, y0, x1, y1); }
	void DrawFilledRectArray( boost::python::list rects );
	inline void DrawOutlinedRect(int x0, int y0, int x1, int y1) { surface()->DrawOutlinedRect(x0, y0, x1, y1); }

	inline void DrawLine(int x0, int y0, int x1, int y1) { surface()->DrawLine(x0, y0, x1, y1); }
	inline void DrawPolyLine(int *px, int *py, int numPoints) { surface()->DrawPolyLine(px, py, numPoints); }

	inline void DrawSetTextFont(HFont font) { surface()->DrawSetTextFont(font); }
	inline void DrawSetTextColor(int r, int g, int b, int a) { surface()->DrawSetTextColor(r, g, b, a); }
	inline void DrawSetTextColor(Color col) { surface()->DrawSetTextColor(col); }
	inline void DrawSetTextPos(int x, int y) { surface()->DrawSetTextPos(x, y); }
	inline void DrawGetTextPos(int& x,int& y) { surface()->DrawGetTextPos(x, y); }
	//inline void DrawPrintText(const wchar_t *text, int textLen, int drawType = FONT_DRAW_DEFAULT ) { surface()->DrawPrintText(text, textLen, (FontDrawType_t)drawType); }
	inline void DrawUnicodeChar(wchar_t wch, int drawType = FONT_DRAW_DEFAULT ) { surface()->DrawUnicodeChar(wch, (FontDrawType_t)drawType); }

	inline void DrawFlushText() {}		// flushes any buffered text (for rendering optimizations)
	inline IHTML *CreateHTMLWindow(vgui::IHTMLEvents *events,VPANEL context) { return surface()->CreateHTMLWindow(events, context); }
	inline void PaintHTMLWindow(vgui::IHTML *htmlwin) { surface()->PaintHTMLWindow(htmlwin); }
	inline void DeleteHTMLWindow(IHTML *htmlwin) { surface()->DeleteHTMLWindow(htmlwin); }

	inline int	 DrawGetTextureId( char const *filename ) { return surface()->DrawGetTextureId(filename); }
	inline bool DrawGetTextureFile(int id, char *filename, int maxlen ) { return surface()->DrawGetTextureFile(id, filename, maxlen); }
	inline void DrawSetTextureFile(int id, const char *filename, int hardwareFilter, bool forceReload) { surface()->DrawSetTextureFile(id, filename, hardwareFilter, forceReload); }
	inline void DrawSetTexture(int id) { surface()->DrawSetTexture(id); }
	inline void DrawGetTextureSize(int id, int &wide, int &tall) { surface()->DrawGetTextureSize(id, wide, tall); }
	inline void DrawTexturedRect(int x0, int y0, int x1, int y1) { surface()->DrawTexturedRect(x0, y0, x1, y1); }
	inline bool IsTextureIDValid(int id) { return surface()->IsTextureIDValid(id); }

	inline int CreateNewTextureID( bool procedural = false ) { return surface()->CreateNewTextureID(procedural); }

	inline void GetScreenSize(int &wide, int &tall) { surface()->GetScreenSize(wide, tall); }
	inline void SetAsTopMost(VPANEL panel, bool state) { surface()->SetAsTopMost(panel, state); }
	inline void BringToFront(VPANEL panel) { surface()->BringToFront(panel); }
	inline void SetForegroundWindow (VPANEL panel) { surface()->SetForegroundWindow(panel); }
	inline void SetPanelVisible(VPANEL panel, bool state) { surface()->SetPanelVisible(panel, state); }
	inline void SetMinimized(VPANEL panel, bool state) { surface()->SetMinimized(panel, state); }
	inline bool IsMinimized(VPANEL panel) { return surface()->IsMinimized(panel); }
	inline void FlashWindow(VPANEL panel, bool state) { surface()->FlashWindow(panel, state); }
	inline void SetTitle(VPANEL panel, const wchar_t *title) { surface()->SetTitle(panel, title); }
	inline void SetAsToolBar(VPANEL panel, bool state) { surface()->SetAsToolBar(panel, state); }

	// windows stuff
	inline void CreatePopup(VPANEL panel, bool minimised, bool showTaskbarIcon = true, bool disabled = false, bool mouseInput = true , bool kbInput = true)
						 { surface()->CreatePopup(panel, minimised, showTaskbarIcon, disabled, mouseInput, kbInput); }
	inline void SwapBuffers(VPANEL panel) { surface()->SwapBuffers(panel); }
	inline void Invalidate(VPANEL panel) { surface()->Invalidate(panel); }
	inline void SetCursor(HCursor cursor) { surface()->SetCursor(cursor); }
	inline bool IsCursorVisible() { return surface()->IsCursorVisible(); }
	inline void ApplyChanges() { surface()->ApplyChanges(); }
	inline bool IsWithin(int x, int y) { return surface()->IsWithin(x, y); }
	inline bool HasFocus() { return surface()->HasFocus(); }

	// returns true if the surface supports minimize & maximize capabilities
	enum SurfaceFeature_t
	{
		ANTIALIASED_FONTS = FONT_FEATURE_ANTIALIASED_FONTS,
		DROPSHADOW_FONTS = FONT_FEATURE_DROPSHADOW_FONTS,
		ESCAPE_KEY			= 3,
		OPENING_NEW_HTML_WINDOWS = 4,
		FRAME_MINIMIZE_MAXIMIZE	 = 5,
		OUTLINE_FONTS = FONT_FEATURE_OUTLINE_FONTS,
		DIRECT_HWND_RENDER		= 7,
	};
	inline bool SupportsFeature( SurfaceFeature_t feature ) { return surface()->SupportsFeature((vgui::ISurface::SurfaceFeature_t)feature); }
	

	// restricts what gets drawn to one panel and it's children
	// currently only works in the game
	inline void RestrictPaintToSinglePanel(VPANEL panel) { surface()->RestrictPaintToSinglePanel(panel); }

	// these two functions obselete, use IInput::SetAppModalSurface() instead
	inline void SetModalPanel(VPANEL panel ) { surface()->SetModalPanel(panel); }
	inline VPANEL GetModalPanel() { return surface()->GetModalPanel(); }

	inline void UnlockCursor() { surface()->UnlockCursor(); }
	inline void LockCursor() { surface()->LockCursor(); }
	inline void SetTranslateExtendedKeys(bool state) { surface()->SetTranslateExtendedKeys(state); }
	inline VPANEL GetTopmostPopup() { return surface()->GetTopmostPopup(); }

	// engine-only focus handling (replacing WM_FOCUS windows handling)
	inline void SetTopLevelFocus(VPANEL panel) { surface()->SetTopLevelFocus(panel); }

	// fonts
	// creates an empty handle to a vgui font.  windows fonts can be add to this via SetFontGlyphSet().
	inline HFont CreateFont() { return surface()->CreateFont(); }

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

	inline bool SetFontGlyphSet(HFont font, const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags, int nRangeMin = 0, int nRangeMax = 0)
				{ return surface()->SetFontGlyphSet(font, windowsFontName, tall, weight, blur, scanlines, flags, nRangeMin, nRangeMax); }

	// adds a custom font file (only supports true type font files (.ttf) for now)
	inline bool AddCustomFontFile(const char *fontFileName) { return surface()->AddCustomFontFile(fontFileName); }

	// returns the details about the font
	inline int GetFontTall(HFont font) { return surface()->GetFontTall(font); }
	inline int GetFontAscent(HFont font, wchar_t wch) { return surface()->GetFontAscent(font, wch); }
	inline bool IsFontAdditive(HFont font) { return surface()->IsFontAdditive(font); }
	inline void GetCharABCwide(HFont font, int ch, int &a, int &b, int &c) { surface()->GetCharABCwide(font, ch, a, b, c); }
	inline int GetCharacterWidth(HFont font, int ch) { return surface()->GetCharacterWidth(font, ch); }
	boost::python::tuple GetTextSize(HFont font, boost::python::object unistr);

	// notify icons?!?
	inline VPANEL GetNotifyPanel() { return surface()->GetNotifyPanel(); }
	inline void SetNotifyIcon(VPANEL context, HTexture icon, VPANEL panelToReceiveMessages, const char *text) 
			{ surface()->SetNotifyIcon(context, icon, panelToReceiveMessages, text); }

	// plays a sound
	inline void PlaySound(const char *fileName) { surface()->PlaySound(fileName); }

	//!! these functions should not be accessed directly, but only through other vgui items
	//!! need to move these to seperate interface
	inline int GetPopupCount() { return surface()->GetPopupCount(); }
	inline VPANEL GetPopup(int index) { return surface()->GetPopup(index); }
	inline bool ShouldPaintChildPanel(VPANEL childPanel) { return surface()->ShouldPaintChildPanel(childPanel); }
	inline bool RecreateContext(VPANEL panel) { return surface()->RecreateContext(panel); }
	inline void AddPanel(VPANEL panel) { surface()->AddPanel(panel); }
	inline void ReleasePanel(VPANEL panel) { surface()->ReleasePanel(panel); }
	inline void MovePopupToFront(VPANEL panel) { surface()->MovePopupToFront(panel); }
	inline void MovePopupToBack(VPANEL panel) { surface()->MovePopupToBack(panel); }

	inline void SolveTraverse(VPANEL panel, bool forceApplySchemeSettings = false) { surface()->SolveTraverse(panel, forceApplySchemeSettings); }
	inline void PaintTraverse(VPANEL panel) { surface()->PaintTraverse(panel); }

	inline void EnableMouseCapture(VPANEL panel, bool state) { surface()->EnableMouseCapture(panel, state); }

	// returns the size of the workspace
	inline void GetWorkspaceBounds(int &x, int &y, int &wide, int &tall) { surface()->GetWorkspaceBounds(x, y, wide, tall); }

	// gets the absolute coordinates of the screen (in windows space)
	inline void GetAbsoluteWindowBounds(int &x, int &y, int &wide, int &tall) { surface()->GetAbsoluteWindowBounds(x, y, wide, tall); }

	// gets the base resolution used in proportional mode
	inline void GetProportionalBase( int &width, int &height ) { surface()->GetProportionalBase(width, height); }

	inline void CalculateMouseVisible() { surface()->CalculateMouseVisible(); }
	inline bool NeedKBInput() { return surface()->NeedKBInput(); }

	inline bool HasCursorPosFunctions() { return surface()->HasCursorPosFunctions(); }
	inline void SurfaceGetCursorPos(int &x, int &y) { surface()->SurfaceGetCursorPos(x, y); }
	inline void SurfaceSetCursorPos(int x, int y) { surface()->SurfaceSetCursorPos(x, y); }


	// SRC only functions!!!
	inline void DrawTexturedLine( const Vertex_t &a, const Vertex_t &b ) { surface()->DrawTexturedLine(a, b); }
	inline void DrawOutlinedCircle(int x, int y, int radius, int segments) { surface()->DrawOutlinedCircle(x, y, radius, segments); }
	void DrawTexturedPolyLine( boost::python::list vertices ); // (Note: this connects the first and last points).
	inline void DrawTexturedSubRect( int x0, int y0, int x1, int y1, float texs0, float text0, float texs1, float text1 ) { surface()->DrawTexturedSubRect(x0, y0, x1, y1, texs0, text0, texs1, text1); }
	void DrawTexturedPolygon(boost::python::list vertices);
	inline const wchar_t *GetTitle(VPANEL panel) { return surface()->GetTitle(panel); }
	inline bool IsCursorLocked( void ) const { return surface()->IsCursorLocked(); }
	inline void SetWorkspaceInsets( int left, int top, int right, int bottom ) { surface()->SetWorkspaceInsets(left, top, right, bottom); }

	// Lower level char drawing code, call DrawGet then pass in info to DrawRender
	inline bool DrawGetUnicodeCharRenderInfo( wchar_t ch, FontCharRenderInfo& info ) { return surface()->DrawGetUnicodeCharRenderInfo(ch, info); }
	inline void DrawRenderCharFromInfo( const FontCharRenderInfo& info ) { surface()->DrawRenderCharFromInfo(info); }

	// global alpha setting functions
	// affect all subsequent draw calls - shouldn't normally be used directly, only in Panel::PaintTraverse()
	inline void DrawSetAlphaMultiplier( float alpha /* [0..1] */ ) { surface()->DrawSetAlphaMultiplier(alpha); }
	inline float DrawGetAlphaMultiplier() { return surface()->DrawGetAlphaMultiplier(); }

	// web browser
	inline void SetAllowHTMLJavaScript( bool state ) { surface()->SetAllowHTMLJavaScript(state); }

	// video mode changing
	inline void OnScreenSizeChanged( int nOldWidth, int nOldHeight ) { surface()->OnScreenSizeChanged(nOldWidth, nOldHeight); }

	inline vgui::HCursor CreateCursorFromFile( char const *curOrAniFile, char const *pPathID = 0 ) { return surface()->CreateCursorFromFile( curOrAniFile, pPathID ); }

	// create IVguiMatInfo object ( IMaterial wrapper in VguiMatSurface, NULL in CWin32Surface )
	inline IVguiMatInfo *DrawGetTextureMatInfoFactory( int id ) { return surface()->DrawGetTextureMatInfoFactory(id); }

	inline void PaintTraverseEx(VPANEL panel, bool paintPopups = false ) { surface()->PaintTraverseEx(panel, paintPopups); }

	inline float GetZPos() const { return surface()->GetZPos(); }

	// From the Xbox
	inline void SetPanelForInput( VPANEL vpanel ) { surface()->SetPanelForInput( vpanel ); }
	inline void DrawFilledRectFade( int x0, int y0, int x1, int y1, unsigned int alpha0, unsigned int alpha1, bool bHorizontal ) { surface()->DrawFilledRectFade( x0, y0, x1, y1, alpha0, alpha1, bHorizontal ); }

	inline void DrawSetTextScale(float sx, float sy) { surface()->DrawSetTextScale( sx, sy ); }
	inline bool SetBitmapFontGlyphSet(HFont font, const char *windowsFontName, float scalex, float scaley, int flags) { return surface()->SetBitmapFontGlyphSet( font, windowsFontName, scalex, scaley, flags ); }
	// adds a bitmap font file
	inline bool AddBitmapFontFile(const char *fontFileName) { return surface()->AddBitmapFontFile(fontFileName); }
	// sets a symbol for the bitmap font
	inline void SetBitmapFontName( const char *pName, const char *pFontFilename ) { surface()->SetBitmapFontName( pName, pFontFilename ); }
	// gets the bitmap font filename
	inline const char *GetBitmapFontName( const char *pName ) { return surface()->GetBitmapFontName(pName); }
	inline void ClearTemporaryFontCache( void ) { surface()->ClearTemporaryFontCache(); }

	inline IImage *GetIconImageForFullPath( char const *pFullPath ) { return surface()->GetIconImageForFullPath(pFullPath); }
	void DrawUnicodeString( boost::python::object unistr, FontDrawType_t drawType = FONT_DRAW_DEFAULT );
	inline void PrecacheFontCharacters(HFont font, wchar_t *pCharacters) { surface()->PrecacheFontCharacters(font, pCharacters); }
	// Console-only.  Get the string to use for the current video mode for layout files.
	inline const char *GetResolutionKey( void ) const { return surface()->GetResolutionKey(); }

	inline const char *GetFontName( HFont font ) { return surface()->GetFontName( font ); }

	inline bool ForceScreenSizeOverride( bool bState, int wide, int tall ) { return surface()->ForceScreenSizeOverride( bState, wide, tall ); }
	// LocalToScreen, ParentLocalToScreen fixups for explicit PaintTraverse calls on Panels not at 0, 0 position
	inline bool ForceScreenPosOffset( bool bState, int x, int y ) { return surface()->ForceScreenPosOffset( bState, x, y ); }
	inline void OffsetAbsPos( int &x, int &y ) { surface()->OffsetAbsPos( x, y ); }

	inline void SetAbsPosForContext( int id, int x, int y ) { surface()->SetAbsPosForContext( id, x, y ); }
	inline void GetAbsPosForContext( int id, int &x, int& y ) { surface()->GetAbsPosForContext( id, x, y ); }

	// Causes fonts to get reloaded, etc.
	inline void ResetFontCaches()  { surface()->ResetFontCaches(); }

	inline bool IsScreenSizeOverrideActive( void ) { return surface()->IsScreenSizeOverrideActive( ); }
	inline bool IsScreenPosOverrideActive( void ) { return surface()->IsScreenPosOverrideActive( ); }

	inline void DestroyTextureID( int id ) { surface()->DestroyTextureID( id ); }

	inline int GetTextureNumFrames( int id ) { return surface()->GetTextureNumFrames( id ); }
	inline void DrawSetTextureFrame( int id, int nFrame, unsigned int *pFrameCache ) { surface()->DrawSetTextureFrame( id, nFrame, pFrameCache ); }

	inline void GetClipRect( int &x0, int &y0, int &x1, int &y1 ) { surface()->GetClipRect( x0, y0, x1, y1 ); }
	inline void SetClipRect( int x0, int y0, int x1, int y1 ) { surface()->SetClipRect( x0, y0, x1, y1 ); }

	//inline void DrawTexturedRectEx( DrawTexturedRectParms_t *pDrawParms )  { return surface()->DrawTexturedRectEx( pDrawParms ); }

	void SetProxyUITeamColor( const Vector &vTeamColor );
	void ClearProxyUITeamColor( void );

};

CWrapSurface *wrapsurface();

//=============================================================================
// IPanel for Python
//=============================================================================
class CWrapIPanel
{
public:
	// methods
	inline void SetPos(VPANEL vguiPanel, int x, int y) { ipanel()->SetPos(vguiPanel, x, y); }
	inline void GetPos(VPANEL vguiPanel, int &x, int &y) { ipanel()->GetPos(vguiPanel, x, y); }
	inline void SetSize(VPANEL vguiPanel, int wide,int tall) { ipanel()->SetSize(vguiPanel, wide, tall); }
	inline void GetSize(VPANEL vguiPanel, int &wide, int &tall) { ipanel()->GetSize(vguiPanel, wide, tall); }
	inline void SetMinimumSize(VPANEL vguiPanel, int wide, int tall) { ipanel()->SetMinimumSize(vguiPanel, wide, tall); }
	inline void GetMinimumSize(VPANEL vguiPanel, int &wide, int &tall) { ipanel()->GetMinimumSize(vguiPanel, wide, tall); }
	inline void SetZPos(VPANEL vguiPanel, int z) { ipanel()->SetZPos(vguiPanel, z); }
	inline int  GetZPos(VPANEL vguiPanel) { return ipanel()->GetZPos(vguiPanel); }

	inline void GetAbsPos(VPANEL vguiPanel, int &x, int &y) { ipanel()->GetAbsPos(vguiPanel, x, y); }
	inline void GetClipRect(VPANEL vguiPanel, int &x0, int &y0, int &x1, int &y1) { ipanel()->GetClipRect(vguiPanel, x0, y0, x1, y1); }
	inline void SetInset(VPANEL vguiPanel, int left, int top, int right, int bottom) { ipanel()->GetInset(vguiPanel, left, top, right, bottom); }
	inline void GetInset(VPANEL vguiPanel, int &left, int &top, int &right, int &bottom) { ipanel()->GetInset(vguiPanel, left, top, right, bottom); }

	inline void SetVisible(VPANEL vguiPanel, bool state) { ipanel()->SetVisible(vguiPanel, state); }
	inline bool IsVisible(VPANEL vguiPanel) { return ipanel()->IsVisible(vguiPanel); }
	inline void SetParent(VPANEL vguiPanel, VPANEL newParent) { ipanel()->SetParent(vguiPanel, newParent); }
	inline int GetChildCount(VPANEL vguiPanel) { return ipanel()->GetChildCount(vguiPanel); }
	inline VPANEL GetChild(VPANEL vguiPanel, int index) { return ipanel()->GetChild(vguiPanel, index); }
	inline VPANEL GetParent(VPANEL vguiPanel) { return ipanel()->GetParent(vguiPanel); }
	inline void MoveToFront(VPANEL vguiPanel) { ipanel()->MoveToFront(vguiPanel); }
	inline void MoveToBack(VPANEL vguiPanel) { ipanel()->MoveToBack(vguiPanel); }
	inline bool HasParent(VPANEL vguiPanel, VPANEL potentialParent) { return ipanel()->HasParent(vguiPanel, potentialParent); }
	inline bool IsPopup(VPANEL vguiPanel) { return ipanel()->IsPopup(vguiPanel); }
	inline void SetPopup(VPANEL vguiPanel, bool state) { ipanel()->SetPopup(vguiPanel, state); }
	inline bool IsFullyVisible( VPANEL vguiPanel ) { return ipanel()->IsFullyVisible(vguiPanel); }

	// gets the scheme this panel uses
	inline HScheme GetScheme(VPANEL vguiPanel) { return ipanel()->GetScheme(vguiPanel); }
	// gets whether or not this panel should scale with screen resolution
	inline bool IsProportional(VPANEL vguiPanel) { return ipanel()->IsProportional(vguiPanel); }

	// input interest
	inline void SetKeyBoardInputEnabled(VPANEL vguiPanel, bool state) { ipanel()->SetKeyBoardInputEnabled(vguiPanel, state); }
	inline void SetMouseInputEnabled(VPANEL vguiPanel, bool state) { ipanel()->SetMouseInputEnabled(vguiPanel, state); }
	inline bool IsKeyBoardInputEnabled(VPANEL vguiPanel) { return ipanel()->IsKeyBoardInputEnabled(vguiPanel); }
	inline bool IsMouseInputEnabled(VPANEL vguiPanel) { return ipanel()->IsMouseInputEnabled(vguiPanel); }

	// calculates the panels current position within the hierarchy
	inline void Solve(VPANEL vguiPanel) { ipanel()->Solve(vguiPanel); }

	// gets names of the object (for debugging purposes)
	inline const char *GetName(VPANEL vguiPanel) { return ipanel()->GetName(vguiPanel); }
	inline const char *GetClassName(VPANEL vguiPanel) { return ipanel()->GetClassName(vguiPanel); }

	// delivers a message to the panel
	inline void SendMessage(VPANEL vguiPanel, KeyValues *params, VPANEL ifromPanel) { ipanel()->SendMessage(vguiPanel, params, ifromPanel); }

	// these pass through to the IClientPanel
	inline void Think(VPANEL vguiPanel) { ipanel()->Think(vguiPanel); }
	inline void PerformApplySchemeSettings(VPANEL vguiPanel) { ipanel()->PerformApplySchemeSettings(vguiPanel); }
	inline void PaintTraverse(VPANEL vguiPanel, bool forceRepaint, bool allowForce = true) { ipanel()->PaintTraverse(vguiPanel, forceRepaint, allowForce); }
	inline void Repaint(VPANEL vguiPanel) { ipanel()->Repaint(vguiPanel); }
	inline VPANEL IsWithinTraverse(VPANEL vguiPanel, int x, int y, bool traversePopups) { return ipanel()->IsWithinTraverse(vguiPanel, x, y, traversePopups); }
	inline void OnChildAdded(VPANEL vguiPanel, VPANEL child) { ipanel()->OnChildAdded(vguiPanel, child); }
	inline void OnSizeChanged(VPANEL vguiPanel, int newWide, int newTall) { ipanel()->OnSizeChanged(vguiPanel, newWide, newTall); }

	inline void InternalFocusChanged(VPANEL vguiPanel, bool lost) { ipanel()->InternalFocusChanged(vguiPanel, lost); }
	inline bool RequestInfo(VPANEL vguiPanel, KeyValues *outputData) { return ipanel()->RequestInfo(vguiPanel, outputData); }
	inline void RequestFocus(VPANEL vguiPanel, int direction = 0) { ipanel()->RequestFocus(vguiPanel, direction); }
	inline bool RequestFocusPrev(VPANEL vguiPanel, VPANEL existingPanel) { return ipanel()->RequestFocusPrev(vguiPanel, existingPanel); }
	inline bool RequestFocusNext(VPANEL vguiPanel, VPANEL existingPanel) { return ipanel()->RequestFocusNext(vguiPanel, existingPanel); }
	inline VPANEL GetCurrentKeyFocus(VPANEL vguiPanel) { return ipanel()->GetCurrentKeyFocus(vguiPanel); }
	inline int GetTabPosition(VPANEL vguiPanel) { return ipanel()->GetTabPosition(vguiPanel); }

	// used by ISurface to store platform-specific data
	inline SurfacePlat *Plat(VPANEL vguiPanel) { return ipanel()->Plat(vguiPanel); }
	inline void SetPlat(VPANEL vguiPanel, SurfacePlat *Plat) { ipanel()->SetPlat(vguiPanel, Plat); }

	// returns a pointer to the vgui controls baseclass Panel *
	// destinationModule needs to be passed in to verify that the returned Panel * is from the same module
	// it must be from the same module since Panel * vtbl may be different in each module
	inline Panel *GetPanel(VPANEL vguiPanel, const char *destinationModule) { return ipanel()->GetPanel(vguiPanel, destinationModule); }

	inline bool IsEnabled(VPANEL vguiPanel) { return ipanel()->IsEnabled(vguiPanel); }
	inline void SetEnabled(VPANEL vguiPanel, bool state) { ipanel()->SetEnabled(vguiPanel, state); }

	// Used by the drag/drop manager to always draw on top
	inline bool IsTopmostPopup( VPANEL vguiPanel) { return ipanel()->IsTopmostPopup(vguiPanel); }
	inline void SetTopmostPopup( VPANEL vguiPanel, bool state ) { ipanel()->SetTopmostPopup(vguiPanel, state); }
};

CWrapIPanel *wrapipanel();

//=============================================================================
// ILocalize for Python
//=============================================================================
class PyLocalize
{
public:
	// adds the contents of a file to the localization table
	inline bool AddFile( const char *fileName, const char *pPathID = NULL, bool bIncludeFallbackSearchPaths = false ) { return g_pVGuiLocalize->AddFile(fileName, pPathID, bIncludeFallbackSearchPaths); }

	// Remove all strings from the table
	inline void RemoveAll() { g_pVGuiLocalize->RemoveAll(); }

	// Finds the localized text for tokenName
	inline wchar_t *Find(char const *tokenName) { return g_pVGuiLocalize->Find(tokenName); }

	// converts an english string to unicode
	// returns the number of wchar_t in resulting string, including null terminator
	boost::python::tuple ConvertANSIToUnicode(const char *ansi);
	// converts an unicode string to an english string
	// unrepresentable characters are converted to system default
	// returns the number of characters in resulting string, including null terminator
	boost::python::tuple ConvertUnicodeToANSI(const wchar_t *unicode);

	// finds the index of a token by token name, INVALID_STRING_INDEX if not found
	inline StringIndex_t FindIndex(const char *tokenName) { return g_pVGuiLocalize->FindIndex(tokenName); }

	// builds a localized formatted string
	// uses the format strings first: %s1, %s2, ...  unicode strings (wchar_t *)
	//inline void ConstructString(wchar_t *unicodeOuput, int unicodeBufferSizeInBytes, wchar_t *formatString, int numFormatParameters, ...) = 0;
	
	// gets the values by the string index
	inline const char *GetNameByIndex(StringIndex_t index) { return g_pVGuiLocalize->GetNameByIndex(index); }
	inline wchar_t *GetValueByIndex(StringIndex_t index) { return g_pVGuiLocalize->GetValueByIndex(index); }

		///////////////////////////////////////////////////////////////////
	// the following functions should only be used by localization editors

	// iteration functions
	inline StringIndex_t GetFirstStringIndex() { return g_pVGuiLocalize->GetFirstStringIndex(); }
	// returns the next index, or INVALID_STRING_INDEX if no more strings available
	inline StringIndex_t GetNextStringIndex(StringIndex_t index) { return g_pVGuiLocalize->GetNextStringIndex(index); }

	// adds a single name/unicode string pair to the table
	inline void AddString( const char *tokenName, wchar_t *unicodeString, const char *fileName ) { g_pVGuiLocalize->AddString(tokenName, unicodeString, fileName); }

	// changes the value of a string
	inline void SetValueByIndex(StringIndex_t index, wchar_t *newValue) { g_pVGuiLocalize->SetValueByIndex(index, newValue); }

	// saves the entire contents of the token tree to the file
	inline bool SaveToFile( const char *fileName ) { return g_pVGuiLocalize->SaveToFile(fileName); }

	// iterates the filenames
	inline int GetLocalizationFileCount() { return g_pVGuiLocalize->GetLocalizationFileCount(); }
	inline const char *GetLocalizationFileName(int index) { return g_pVGuiLocalize->GetLocalizationFileName(index); }

	// returns the name of the file the specified localized string is stored in
	inline const char *GetFileNameByIndex(StringIndex_t index) { return g_pVGuiLocalize->GetFileNameByIndex(index); }

	// for development only, reloads localization files
	inline void ReloadLocalizationFiles( ) { g_pVGuiLocalize->ReloadLocalizationFiles(); }

	// need to replace the existing ConstructString with this
	boost::python::object ConstructString(const char *tokenName, KeyValues *localizationVariables);
	boost::python::object ConstructString(StringIndex_t unlocalizedTextSymbol, KeyValues *localizationVariables);
};

extern PyLocalize *g_pylocalize;

//=============================================================================
// Misc
//=============================================================================
bool	PyIsGameUIVisible();
VPANEL  PyGetPanel( VGuiPanel_t type );

// emporary function until everything is ported over to the new html based menu
void PyGameUICommand( const char *command );

#endif // SRCPY_VGUI_H