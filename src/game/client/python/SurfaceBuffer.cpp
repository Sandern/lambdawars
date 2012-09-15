#include "cbase.h"

#include <Color.h>
#include <vgui/IPanel.h>
#include <vgui/ISystem.h>

#include "SurfaceBuffer.h"
#include <vgui_controls/Panel.h>
#include <vgui_controls/Controls.h>

#include "src_python_vgui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static vgui::CSurfaceBuffered s_VGuiSurfaceBuffered;
vgui::ISurface *vgui::g_pVGuiSurfaceBuffered = &s_VGuiSurfaceBuffered;

extern vgui::ISurface *g_pVGuiSurface;

bool g_bUseSurfaceBuffer = false;


CUtlVector<vgui::BaseBufferCall *> *g_pActiveSurfaceBuffer = NULL;

// Enabling buffering
static vgui::ISurface *pRealVGuiSurface = NULL;
static ConVar cl_recordsurfacebuffer( "cl_recordsurfacebuffer", "0", 0 );

void SBufferStartRecording( CUtlVector< vgui::BaseBufferCall * > *pCallBuffer )
{
	if( pCallBuffer )
	{
		// Swap the real surface with the recording surface version
		pRealVGuiSurface = g_pVGuiSurface;
		g_pVGuiSurface = vgui::g_pVGuiSurfaceBuffered;
		g_pActiveSurfaceBuffer = pCallBuffer;
	}
}

bool SBufferIsRecording()
{
	return g_pActiveSurfaceBuffer != NULL;
}

void SBufferFinishRecording()
{
	if( pRealVGuiSurface )
	{
		// Switch back to the real surface and set the active record buffer to NULL
		g_pVGuiSurface = pRealVGuiSurface;
		pRealVGuiSurface = NULL;
		g_pActiveSurfaceBuffer = NULL;
	}
}

using namespace vgui;

// No arguments
template <class RV>
class BufferCall : public BaseBufferCall
{
public:
	typedef RV (ISurface::*SurfacePointer) ();

	BufferCall( SurfacePointer ptr ) : fnptr(ptr) {}

	void Draw() { ((*g_pVGuiSurface).*fnptr)( ); }

private:
	SurfacePointer fnptr;
};

// One argument
template <class RV, class Arg1>
class BufferCallArg1 : public BaseBufferCall
{
public:
	typedef RV (ISurface::*SurfacePointer) (Arg1);

	BufferCallArg1( SurfacePointer ptr, Arg1 a1 ) : fnptr(ptr), a1(a1) {}

	void Draw() { ((*g_pVGuiSurface).*fnptr)( a1 ); }

private:
	SurfacePointer fnptr;
	Arg1 a1;
};

// Two arguments
template <class RV, class Arg1, class Arg2>
class BufferCallArg2 : public BaseBufferCall
{
public:
	typedef RV (ISurface::*SurfacePointer) (Arg1, Arg2);

	BufferCallArg2( SurfacePointer ptr, Arg1 a1, Arg2 a2 ) : fnptr(ptr),
		a1(a1), a2(a2) {}

	void Draw() { ((*g_pVGuiSurface).*fnptr)( a1, a2 ); }

private:
	SurfacePointer fnptr;
	Arg1 a1; 
	Arg2 a2;
};

// Three arguments
template <class RV, class Arg1, class Arg2, class Arg3>
class BufferCallArg3 : public BaseBufferCall
{
public:
	typedef RV (ISurface::*SurfacePointer) (Arg1, Arg2, Arg3);

	BufferCallArg3( SurfacePointer ptr, Arg1 a1, Arg2 a2, Arg3 a3 ) : fnptr(ptr),
		a1(a1), a2(a2), a3(a3) {}

	void Draw() { ((*g_pVGuiSurface).*fnptr)( a1, a2, a3 ); }

private:
	SurfacePointer fnptr;
	Arg1 a1; 
	Arg2 a2;
	Arg3 a3;
};

// Four arguments
template <class RV, class Arg1, class Arg2, class Arg3, class Arg4>
class BufferCallArg4 : public BaseBufferCall
{
public:
	typedef RV (ISurface::*SurfacePointer) (Arg1, Arg2, Arg3, Arg4);

	BufferCallArg4( SurfacePointer ptr, Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4 ) : fnptr(ptr),
		a1(a1), a2(a2), a3(a3), a4(a4) {}

	void Draw() { ((*g_pVGuiSurface).*fnptr)( a1, a2, a3, a4 ); }

private:
	SurfacePointer fnptr;
	Arg1 a1; 
	Arg2 a2;
	Arg3 a3;
	Arg4 a4;
};

// Seven arguments

template <class RV, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7>
class BufferCallArg7 : public BaseBufferCall
{
public:
	typedef RV (ISurface::*SurfacePointer) (Arg1, Arg2, Arg3, Arg4, Arg5 a5, Arg6 a6, Arg7 a7);

	BufferCallArg7( SurfacePointer ptr, Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4, Arg5 a5, Arg6 a6, Arg7 a7 ) : fnptr(ptr),
		a1(a1), a2(a2), a3(a3), a4(a4), a5(a5), a6(a6), a7(a7) {}

	void Draw() { ((*g_pVGuiSurface).*fnptr)( a1, a2, a3, a4, a5, a6, a7 ); }

private:
	SurfacePointer fnptr;
	Arg1 a1; Arg2 a2; Arg3 a3; Arg4 a4;
	Arg5 a5; Arg6 a6; Arg7 a7;
};

// Eight arguments
template <class RV, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5, class Arg6, class Arg7, class Arg8>
class BufferCallArg8 : public BaseBufferCall
{
public:
	typedef RV (ISurface::*SurfacePointer) (Arg1, Arg2, Arg3, Arg4, Arg5 a5, Arg6 a6, Arg7 a7, Arg8 a8);

	BufferCallArg8( SurfacePointer ptr, Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4, Arg5 a5, Arg6 a6, Arg7 a7, Arg8 a8 ) : fnptr(ptr),
		a1(a1), a2(a2), a3(a3), a4(a4), a5(a5), a6(a6), a7(a7), a8(a8) {}

	void Draw() { ((*g_pVGuiSurface).*fnptr)( a1, a2, a3, a4, a5, a6, a7, a8 ); }

private:
	SurfacePointer fnptr;
	Arg1 a1; Arg2 a2; Arg3 a3; Arg4 a4;
	Arg5 a5; Arg6 a6; Arg7 a7; Arg8 a8;
};

// Specific buffer call classes
class BufferCallTexturedLine : public BaseBufferCall
{
public:
	BufferCallTexturedLine( const Vertex_t &a, const Vertex_t &b ) : a(a), b(b) {}
	void Draw() { g_pVGuiSurface->DrawTexturedLine(a, b); }
private:
	 Vertex_t a, b;
};

class BufferCallFilledRectArray : public BaseBufferCall
{
public:
	BufferCallFilledRectArray( IntRect *pRects, int numRects ) : rects(*pRects), numRects(numRects) {}
	void Draw() { g_pVGuiSurface->DrawFilledRectArray(&rects, numRects); }
private:
	 IntRect rects; int numRects;
};

class BufferCallPolyLine : public BaseBufferCall
{
public:
	BufferCallPolyLine( int *pxfrom, int *pyfrom, int numPoints ) : numPoints(numPoints) 
	{
		px = (int *)malloc(sizeof(int)*numPoints);
		py = (int *)malloc(sizeof(int)*numPoints);
		memcpy(px, pxfrom, sizeof(int)*numPoints);
		memcpy(py, pyfrom, sizeof(int)*numPoints);
	}
	~BufferCallPolyLine()
	{
		free(px); free(py);
	}
	void Draw() { g_pVGuiSurface->DrawPolyLine(px, py, numPoints); }
private:
	int *px, *py;
	int numPoints;
};

class BufferCallPrintText : public BaseBufferCall
{
public:
	BufferCallPrintText( const wchar_t *textsrc, int textLen, FontDrawType_t drawType ) : drawType(drawType) 
	{
		text = (wchar_t *)malloc(sizeof(wchar_t)*textLen);
		memcpy(text, textsrc, sizeof(wchar_t)*textLen);
	}
	~BufferCallPrintText()
	{
		free(text);
	}
	void Draw() { g_pVGuiSurface->DrawPrintText(text, textLen, drawType); }
private:
	wchar_t *text;
	int textLen; FontDrawType_t drawType;
};

class BufferCallTexturedPolyLine : public BaseBufferCall
{
public:
	BufferCallTexturedPolyLine( const Vertex_t *pfrom, int n ) : n(n) 
	{
		p = (Vertex_t *)malloc(sizeof(Vertex_t)*n);
		memcpy(p, pfrom, sizeof(Vertex_t)*n);
	}
	~BufferCallTexturedPolyLine()
	{
		free(p);
	}
	void Draw() { g_pVGuiSurface->DrawTexturedPolyLine( p, n ); }
private:
	Vertex_t *p; int n;
};

class BufferCallTexturedPolygon : public BaseBufferCall
{
public:
	BufferCallTexturedPolygon( int n, Vertex_t *pVerticesFrom, bool bClipVertices ) : n(n), bClipVertices(bClipVertices)
	{
		pVertices = (Vertex_t *)malloc(sizeof(Vertex_t)*n);
		memcpy(pVertices, pVerticesFrom, sizeof(Vertex_t)*n);
	}
	~BufferCallTexturedPolygon()
	{
		free(pVertices);
	}
	void Draw() { g_pVGuiSurface->DrawTexturedPolygon( n, pVertices, bClipVertices ); }
private:
	Vertex_t *pVertices; int n; bool bClipVertices;
};

class BufferCallUnicodeString : public BaseBufferCall
{
public:
	BufferCallUnicodeString( const wchar_t *pwString, FontDrawType_t drawType ) : drawType(drawType) 
	{
		int len = Q_wcslen(pwString);
		text = (wchar_t *)malloc(sizeof(wchar_t)*len+1);
		memcpy(text, pwString, sizeof(wchar_t)*len);
		text[len] = '\0';
	}
	~BufferCallUnicodeString()
	{
		free(text);
	}
	void Draw() { g_pVGuiSurface->DrawUnicodeString(text, drawType); }
private:
	wchar_t *text;
	FontDrawType_t drawType;
};

class BufferCallSetVisible : public BaseBufferCall
{
public:
	BufferCallSetVisible( VPANEL panel, bool bVisible ) : panel(panel), visible(bVisible) {}

	void Draw() { ipanel()->SetVisible(panel, visible); }
private:
	VPANEL panel;
	bool visible;
};

// Functions
VPANEL CSurfaceBuffered::GetEmbeddedPanel() 
{ 
	return pRealVGuiSurface->GetEmbeddedPanel(); 
}

void CSurfaceBuffered::SetEmbeddedPanel( VPANEL pPanel ) 
{ 
	pRealVGuiSurface->SetEmbeddedPanel( pPanel ); 

	g_pActiveSurfaceBuffer->AddToTail( 
		new BufferCallArg1<void, VPANEL>( &ISurface::SetEmbeddedPanel, pPanel )
	);
}

void CSurfaceBuffered::PushMakeCurrent(VPANEL panel, bool useInsets) 
{ 
	pRealVGuiSurface->PushMakeCurrent( panel, useInsets ); 

	g_pActiveSurfaceBuffer->AddToTail( 
		new BufferCallArg2<void, VPANEL, bool>( &ISurface::PushMakeCurrent, panel, useInsets )
	);
}

void CSurfaceBuffered::PopMakeCurrent(VPANEL panel) 
{ 
	pRealVGuiSurface->PopMakeCurrent( panel );

	g_pActiveSurfaceBuffer->AddToTail( 
		new BufferCallArg1<void, VPANEL>( &ISurface::PopMakeCurrent, panel )
	);
}

// rendering functions
void CSurfaceBuffered::DrawSetColor(int r, int g, int b, int a) 
{ 
	// BUFFER CALL
	pRealVGuiSurface->DrawSetColor(r, g, b, a); 

	g_pActiveSurfaceBuffer->AddToTail( 
		new BufferCallArg4<void, int, int, int, int>( &ISurface::DrawSetColor, r, g, b, a )
	);
}
void CSurfaceBuffered::DrawSetColor(Color col) 
{ 
	// BUFFER CALL
	pRealVGuiSurface->DrawSetColor(col); 

	g_pActiveSurfaceBuffer->AddToTail( 
		new BufferCallArg1<void, Color>( &ISurface::DrawSetColor, col )
	);
}

void CSurfaceBuffered::DrawFilledRect(int x0, int y0, int x1, int y1)
{ 
	// BUFFER CALL
	pRealVGuiSurface->DrawFilledRect(x0, y0, x1, y1); 

	g_pActiveSurfaceBuffer->AddToTail( 
		new BufferCallArg4<void, int, int, int, int>( &ISurface::DrawFilledRect, x0, y0, x1, y1 )
	);
}

void CSurfaceBuffered::DrawFilledRectArray( IntRect *pRects, int numRects ) 
{ 
	// BUFFER CALL
	pRealVGuiSurface->DrawFilledRectArray( pRects, numRects); 

	g_pActiveSurfaceBuffer->AddToTail( 
		new BufferCallFilledRectArray( pRects, numRects )
	);
}
void CSurfaceBuffered::DrawOutlinedRect(int x0, int y0, int x1, int y1) 
{ 
	// BUFFER CALL
	pRealVGuiSurface->DrawOutlinedRect(x0, y0, x1, y1); 

	g_pActiveSurfaceBuffer->AddToTail( 
		new BufferCallArg4<void, int, int, int, int>( &ISurface::DrawOutlinedRect, x0, y0, x1, y1 )
	);
}

void CSurfaceBuffered::DrawLine(int x0, int y0, int x1, int y1) 
{
	// BUFFER CALL
	pRealVGuiSurface->DrawLine(x0, y0, x1, y1); 

	g_pActiveSurfaceBuffer->AddToTail( 
		new BufferCallArg4<void, int, int, int, int>( &ISurface::DrawLine, x0, y0, x1, y1 )
	);
}

void CSurfaceBuffered::DrawPolyLine(int *px, int *py, int numPoints) 
{ 
	// BUFFER CALL
	pRealVGuiSurface->DrawPolyLine(px, py, numPoints);

	g_pActiveSurfaceBuffer->AddToTail( 
		new BufferCallPolyLine( px, py, numPoints )
	);
	
}

void CSurfaceBuffered::DrawSetTextFont(HFont font) 
{ 
	// BUFFER CALL
	pRealVGuiSurface->DrawSetTextFont(font); 

	g_pActiveSurfaceBuffer->AddToTail( 
		new BufferCallArg1<void, HFont>( &ISurface::DrawSetTextFont, font )
	);
}

void CSurfaceBuffered::DrawSetTextColor(int r, int g, int b, int a) 
{ 
	// BUFFER CALL
	pRealVGuiSurface->DrawSetTextColor(r, g, b, a); 

	g_pActiveSurfaceBuffer->AddToTail( 
		new BufferCallArg4<void, int, int, int, int>( &ISurface::DrawSetTextColor, r, g, b, a )
	);
}

void CSurfaceBuffered::DrawSetTextColor(Color col) 
{ 
	// BUFFER CALL
	pRealVGuiSurface->DrawSetTextColor(col); 

	g_pActiveSurfaceBuffer->AddToTail( 
		new BufferCallArg1<void, Color>( &ISurface::DrawSetTextColor, col )
	);
}

void CSurfaceBuffered::DrawSetTextPos(int x, int y) 
{
	// BUFFER CALL
	pRealVGuiSurface->DrawSetTextPos(x, y); 

	g_pActiveSurfaceBuffer->AddToTail( 
		new BufferCallArg2<void, int, int>( &ISurface::DrawSetTextPos, x, y )
	);
}

void CSurfaceBuffered::DrawGetTextPos(int& x,int& y) 
{ 
	// NOT BUFFERED
	pRealVGuiSurface->DrawGetTextPos(x, y); 
	
}
void CSurfaceBuffered::DrawPrintText(const wchar_t *text, int textLen, FontDrawType_t drawType ) 
{ 
	// BUFFER CALL 
	pRealVGuiSurface->DrawPrintText(text, textLen, drawType); 

	g_pActiveSurfaceBuffer->AddToTail( 
		new BufferCallPrintText( text, textLen, drawType )
	);
	
}

void CSurfaceBuffered::DrawUnicodeChar(wchar_t wch, FontDrawType_t drawType ) 
{ 
	// BUFFER CALL
	pRealVGuiSurface->DrawUnicodeChar(wch, drawType); 

	g_pActiveSurfaceBuffer->AddToTail( 
		new BufferCallArg2<void, wchar_t, FontDrawType_t>( &ISurface::DrawUnicodeChar, wch, drawType )
	);
}

void CSurfaceBuffered::DrawFlushText() 
{ 
	// BUFFER CALL
	pRealVGuiSurface->DrawFlushText(); 

	g_pActiveSurfaceBuffer->AddToTail( 
		new BufferCall<void>( &ISurface::DrawFlushText )
	);
}		

IHTML *CSurfaceBuffered::CreateHTMLWindow(vgui::IHTMLEvents *events,VPANEL context) { return pRealVGuiSurface->CreateHTMLWindow(events, context); }
void CSurfaceBuffered::PaintHTMLWindow(vgui::IHTML *htmlwin) { pRealVGuiSurface->PaintHTMLWindow(htmlwin); }
void CSurfaceBuffered::DeleteHTMLWindow(IHTML *htmlwin) { pRealVGuiSurface->DeleteHTMLWindow(htmlwin); }

int	 CSurfaceBuffered::DrawGetTextureId( char const *filename ) { return pRealVGuiSurface->DrawGetTextureId(filename); }
bool CSurfaceBuffered::DrawGetTextureFile(int id, char *filename, int maxlen ) { return pRealVGuiSurface->DrawGetTextureFile(id, filename, maxlen); }
void CSurfaceBuffered::DrawSetTextureFile(int id, const char *filename, int hardwareFilter, bool forceReload) { pRealVGuiSurface->DrawSetTextureFile(id, filename, hardwareFilter, forceReload); }

void CSurfaceBuffered::DrawSetTextureRGBA(int id, const unsigned char *rgba, int wide, int tall) { pRealVGuiSurface->DrawSetTextureRGBA(id, rgba, wide, tall); }

void CSurfaceBuffered::DrawSetTexture(int id) 
{ 
	// BUFFER CALL
	pRealVGuiSurface->DrawSetTexture(id);

	g_pActiveSurfaceBuffer->AddToTail( 
		new BufferCallArg1<void, int>( &ISurface::DrawSetTexture, id )
	);
}
void CSurfaceBuffered::DrawGetTextureSize(int id, int &wide, int &tall) 
{ 
	// NOT BUFFERED
	pRealVGuiSurface->DrawGetTextureSize(id, wide, tall); 
}

void CSurfaceBuffered::DrawTexturedRect(int x0, int y0, int x1, int y1) 
{ 
	// BUFFER CALL
	pRealVGuiSurface->DrawTexturedRect(x0, y0, x1, y1); 

	g_pActiveSurfaceBuffer->AddToTail( 
		new BufferCallArg4<void, int, int, int, int>( &ISurface::DrawTexturedRect, x0, y0, x1, y1 )
	);
}
bool CSurfaceBuffered::IsTextureIDValid(int id) { return pRealVGuiSurface->IsTextureIDValid(id); }

int CSurfaceBuffered::CreateNewTextureID( bool procedural ) { return pRealVGuiSurface->CreateNewTextureID(procedural); }

void CSurfaceBuffered::GetScreenSize(int &wide, int &tall) { pRealVGuiSurface->GetScreenSize(wide, tall); }
void CSurfaceBuffered::SetAsTopMost(VPANEL panel, bool state) { pRealVGuiSurface->SetAsTopMost(panel, state); }
void CSurfaceBuffered::BringToFront(VPANEL panel) { pRealVGuiSurface->BringToFront(panel); }
void CSurfaceBuffered::SetForegroundWindow (VPANEL panel) { pRealVGuiSurface->SetForegroundWindow(panel); }
void CSurfaceBuffered::SetPanelVisible(VPANEL panel, bool state) 
{ 
	pRealVGuiSurface->SetPanelVisible(panel, state); 
}

void CSurfaceBuffered::SetMinimized(VPANEL panel, bool state) { pRealVGuiSurface->SetMinimized(panel, state); }
bool CSurfaceBuffered::IsMinimized(VPANEL panel) { return pRealVGuiSurface->IsMinimized(panel); }
void CSurfaceBuffered::FlashWindow(VPANEL panel, bool state) { pRealVGuiSurface->FlashWindow(panel, state); }
void CSurfaceBuffered::SetTitle(VPANEL panel, const wchar_t *title) { pRealVGuiSurface->SetTitle(panel, title); }
void CSurfaceBuffered::SetAsToolBar(VPANEL panel, bool state) { pRealVGuiSurface->SetAsToolBar(panel, state); }

// windows stuff
void CSurfaceBuffered::CreatePopup(VPANEL panel, bool minimised, bool showTaskbarIcon, bool disabled, bool mouseInput , bool kbInput)
                     { pRealVGuiSurface->CreatePopup(panel, minimised, showTaskbarIcon, disabled, mouseInput, kbInput); }
void CSurfaceBuffered::SwapBuffers(VPANEL panel) { pRealVGuiSurface->SwapBuffers(panel); }
void CSurfaceBuffered::Invalidate(VPANEL panel) 
{ 
	// BUFFER CALL
	pRealVGuiSurface->Invalidate(panel);
	
#if 0
	g_pActiveSurfaceBuffer->AddToTail( new BufferCallSetVisible(panel, true) );
	g_pActiveSurfaceBuffer->AddToTail( 
		new BufferCallArg1<void, VPANEL>( &ISurface::Invalidate, panel )
	);
	g_pActiveSurfaceBuffer->AddToTail( new BufferCallSetVisible(panel, true) );
#endif // 0
}
void CSurfaceBuffered::SetCursor(HCursor cursor) { pRealVGuiSurface->SetCursor(cursor); }
bool CSurfaceBuffered::IsCursorVisible() { return pRealVGuiSurface->IsCursorVisible(); }
void CSurfaceBuffered::ApplyChanges() { pRealVGuiSurface->ApplyChanges(); }
bool CSurfaceBuffered::IsWithin(int x, int y) { return pRealVGuiSurface->IsWithin(x, y); }
bool CSurfaceBuffered::HasFocus() { return pRealVGuiSurface->HasFocus(); }

// returns true if the surface supports minimize & maximize capabilities
bool CSurfaceBuffered::SupportsFeature(ISurface::SurfaceFeature_t feature) { return pRealVGuiSurface->SupportsFeature(feature); }

// restricts what gets drawn to one panel and it's children
// currently only works in the game
void CSurfaceBuffered::RestrictPaintToSinglePanel(VPANEL panel, bool bForceAllowNonModalSurface) { pRealVGuiSurface->RestrictPaintToSinglePanel(panel, bForceAllowNonModalSurface); }

// these two functions obselete, use IInput::SetAppModalSurface() instead
void CSurfaceBuffered::SetModalPanel(VPANEL panel ) { pRealVGuiSurface->SetModalPanel(panel); }
VPANEL CSurfaceBuffered::GetModalPanel() { return pRealVGuiSurface->GetModalPanel(); }

void CSurfaceBuffered::UnlockCursor() { pRealVGuiSurface->UnlockCursor(); }
void CSurfaceBuffered::LockCursor() { pRealVGuiSurface->LockCursor(); }
void CSurfaceBuffered::SetTranslateExtendedKeys(bool state) { pRealVGuiSurface->SetTranslateExtendedKeys(state); }
VPANEL CSurfaceBuffered::GetTopmostPopup() { return pRealVGuiSurface->GetTopmostPopup(); }

// engine-only focus handling (replacing WM_FOCUS windows handling)
void CSurfaceBuffered::SetTopLevelFocus(VPANEL panel) { pRealVGuiSurface->SetTopLevelFocus(panel); }

// fonts
// creates an empty handle to a vgui font.  windows fonts can be add to this via SetFontGlyphSet().
HFont CSurfaceBuffered::CreateFont() { return pRealVGuiSurface->CreateFont(); }

bool CSurfaceBuffered::SetFontGlyphSet(HFont font, const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags, int nRangeMin, int nRangeMax)
            { return pRealVGuiSurface->SetFontGlyphSet(font, windowsFontName, tall, weight, blur, scanlines, flags, nRangeMin, nRangeMax); }

// adds a custom font file (only supports true type font files (.ttf) for now)
bool CSurfaceBuffered::AddCustomFontFile(const char *fontFileName) { return pRealVGuiSurface->AddCustomFontFile(fontFileName); }

// returns the details about the font
int CSurfaceBuffered::GetFontTall(HFont font) { return pRealVGuiSurface->GetFontTall(font); }
int CSurfaceBuffered::GetFontAscent(HFont font, wchar_t wch) { return pRealVGuiSurface->GetFontAscent(font, wch); }
bool CSurfaceBuffered::IsFontAdditive(HFont font) { return pRealVGuiSurface->IsFontAdditive(font); }
void CSurfaceBuffered::GetCharABCwide(HFont font, int ch, int &a, int &b, int &c) { pRealVGuiSurface->GetCharABCwide(font, ch, a, b, c); }
int CSurfaceBuffered::GetCharacterWidth(HFont font, int ch) { return pRealVGuiSurface->GetCharacterWidth(font, ch); }
void CSurfaceBuffered::GetTextSize(HFont font, const wchar_t *text, int &wide, int &tall) { pRealVGuiSurface->GetTextSize(font, text, wide, tall); }

// notify icons?!?
VPANEL CSurfaceBuffered::GetNotifyPanel() { return pRealVGuiSurface->GetNotifyPanel(); }
void CSurfaceBuffered::SetNotifyIcon(VPANEL context, HTexture icon, VPANEL panelToReceiveMessages, const char *text) 
        { pRealVGuiSurface->SetNotifyIcon(context, icon, panelToReceiveMessages, text); }

// plays a sound
void CSurfaceBuffered::PlaySound(const char *fileName) { pRealVGuiSurface->PlaySound(fileName); }

//!! these functions should not be accessed directly, but only through other vgui items
//!! need to move these to seperate interface
int CSurfaceBuffered::GetPopupCount() { return pRealVGuiSurface->GetPopupCount(); }
VPANEL CSurfaceBuffered::GetPopup(int index) { return pRealVGuiSurface->GetPopup(index); }
bool CSurfaceBuffered::ShouldPaintChildPanel(VPANEL childPanel) { return pRealVGuiSurface->ShouldPaintChildPanel(childPanel); }
bool CSurfaceBuffered::RecreateContext(VPANEL panel) { return pRealVGuiSurface->RecreateContext(panel); }
void CSurfaceBuffered::AddPanel(VPANEL panel) { pRealVGuiSurface->AddPanel(panel); }
void CSurfaceBuffered::ReleasePanel(VPANEL panel) { pRealVGuiSurface->ReleasePanel(panel); }
void CSurfaceBuffered::MovePopupToFront(VPANEL panel) { pRealVGuiSurface->MovePopupToFront(panel); }
void CSurfaceBuffered::MovePopupToBack(VPANEL panel) { pRealVGuiSurface->MovePopupToBack(panel); }

void CSurfaceBuffered::SolveTraverse(VPANEL panel, bool forceApplySchemeSettings) 
{ 
#if 0
	g_pActiveSurfaceBuffer->AddToTail( new BufferCallSetVisible(panel, true) );
	g_pActiveSurfaceBuffer->AddToTail( 
		new BufferCallArg2<void, VPANEL, bool>( &ISurface::SolveTraverse, panel, forceApplySchemeSettings )
	);
	g_pActiveSurfaceBuffer->AddToTail( new BufferCallSetVisible(panel, false) );
#endif // 0

	// Save buffer
	CUtlVector<vgui::BaseBufferCall *> *pBuffer = g_pActiveSurfaceBuffer;
	SBufferFinishRecording();

	// BUFFER CALL
	g_pVGuiSurface->SolveTraverse(panel, forceApplySchemeSettings); 

	// Restore
	SBufferStartRecording( pBuffer );
}


class BufferCallPaintTraverse : public BaseBufferCall
{
public:
	BufferCallPaintTraverse( VPANEL panel ) : panel(panel) 
	{
		Panel *pPanel = ipanel()->GetPanel(panel, GetControlsModuleName());
		if( pPanel )
		{
			pPanel->GetPos(posx, posy);
			pPanel->GetSize(sizex, sizey);

			m_bPaintBackground = pPanel->_flags.IsFlagSet( vgui::Panel::PAINT_BACKGROUND_ENABLED );
			m_bPaint = pPanel->_flags.IsFlagSet( vgui::Panel::PAINT_ENABLED );
			m_bPaintBorder = pPanel->_flags.IsFlagSet( vgui::Panel::PAINT_BORDER_ENABLED );

			PyPanel *pPyPanel = dynamic_cast< PyPanel * >( pPanel );
			if( pPyPanel )
			{
				if( pPyPanel->m_PaintCallBuffer.m_SurfaceCallRecordFrameTime != vgui::system()->GetFrameTime() )
				{
					pPyPanel->m_PaintCallBuffer.m_bShouldRecordBuffer = true; 
					//pPyPanel->m_PaintBackgroundCallBuffer.m_bShouldRecordBuffer = true; 
				}
				if( pPyPanel->m_PaintBackgroundCallBuffer.m_SurfaceCallRecordFrameTime != vgui::system()->GetFrameTime() )
				{
					//pPyPanel->m_PaintCallBuffer.m_bShouldRecordBuffer = true; 
					pPyPanel->m_PaintBackgroundCallBuffer.m_bShouldRecordBuffer = true; 
				}
			}
		}
	}
	void Draw() 
	{ 
		bool bPaintBackground, bPaint, bPaintBorder;
		int rposx, rposy, rsizex, rsizey;
		VPANEL rparent;

		Panel *pPanel = ipanel()->GetPanel(panel, GetControlsModuleName());

		if( pPanel )
		{
			bPaintBackground = pPanel->_flags.IsFlagSet( vgui::Panel::PAINT_BACKGROUND_ENABLED );
			bPaint = pPanel->_flags.IsFlagSet( vgui::Panel::PAINT_ENABLED );
			bPaintBorder = pPanel->_flags.IsFlagSet( vgui::Panel::PAINT_BORDER_ENABLED );

			pPanel->GetPos(rposx, rposy);
			pPanel->GetSize(rsizex, rsizey);
			//rparent = pPanel->GetVParent();

			pPanel->SetPos(posx, posy);
			pPanel->SetSize(sizex, sizey);
			//pPanel->SetParent(parent);

			pPanel->SetPaintBackgroundEnabled( m_bPaintBackground );
			pPanel->SetPaintEnabled( m_bPaint );
			pPanel->SetPaintBorderEnabled( m_bPaintBorder );
		}
		else
		{
			bPaintBackground = bPaint = bPaintBorder = false;
			rposx = rposy = rsizex = rsizey = 0;
			rparent = 0;
		}

		// SolveTraverse will update the panel in relation to the pos and size
		g_pVGuiSurface->Invalidate(panel);
		g_pVGuiSurface->SolveTraverse(panel);

		g_pVGuiSurface->PaintTraverse(panel); 

		if( pPanel )
		{
			pPanel->SetPos(rposx, rposy);
			pPanel->SetSize(rsizex, rsizey);
			//pPanel->SetParent(rparent);

			pPanel->SetPaintBackgroundEnabled( bPaintBackground );
			pPanel->SetPaintEnabled( bPaint );
			pPanel->SetPaintBorderEnabled( bPaintBorder );
		}
	}
private:
	 VPANEL panel, parent;
	 int posx, posy, sizex, sizey;
	 bool m_bPaint, m_bPaintBackground, m_bPaintBorder;
};

void CSurfaceBuffered::PaintTraverse(VPANEL panel) 
{ 
	g_pActiveSurfaceBuffer->AddToTail( new BufferCallSetVisible(panel, true) );
	g_pActiveSurfaceBuffer->AddToTail( 
		new BufferCallPaintTraverse(panel)
	);
	g_pActiveSurfaceBuffer->AddToTail( new BufferCallSetVisible(panel, false) );

	// Save buffer
	CUtlVector<vgui::BaseBufferCall *> *pBuffer = g_pActiveSurfaceBuffer;
	SBufferFinishRecording();

	// BUFFER CALL
	g_pVGuiSurface->PaintTraverse(panel); 

	// Restore
	SBufferStartRecording( pBuffer );
}

void CSurfaceBuffered::EnableMouseCapture(VPANEL panel, bool state) { pRealVGuiSurface->EnableMouseCapture(panel, state); }

// returns the size of the workspace
void CSurfaceBuffered::GetWorkspaceBounds(int &x, int &y, int &wide, int &tall) { pRealVGuiSurface->GetWorkspaceBounds(x, y, wide, tall); }

// gets the absolute coordinates of the screen (in windows space)
void CSurfaceBuffered::GetAbsoluteWindowBounds(int &x, int &y, int &wide, int &tall) { pRealVGuiSurface->GetAbsoluteWindowBounds(x, y, wide, tall); }

// gets the base resolution used in proportional mode
void CSurfaceBuffered::GetProportionalBase( int &width, int &height ) { pRealVGuiSurface->GetProportionalBase(width, height); }

void CSurfaceBuffered::CalculateMouseVisible() { pRealVGuiSurface->CalculateMouseVisible(); }
bool CSurfaceBuffered::NeedKBInput() { return pRealVGuiSurface->NeedKBInput(); }

bool CSurfaceBuffered::HasCursorPosFunctions() { return pRealVGuiSurface->HasCursorPosFunctions(); }
void CSurfaceBuffered::SurfaceGetCursorPos(int &x, int &y) { pRealVGuiSurface->SurfaceGetCursorPos(x, y); }
void CSurfaceBuffered::SurfaceSetCursorPos(int x, int y) { pRealVGuiSurface->SurfaceSetCursorPos(x, y); }


// SRC only functions!!!
void CSurfaceBuffered::DrawTexturedLine( const Vertex_t &a, const Vertex_t &b ) 
{ 
	// BUFFER CALL
	pRealVGuiSurface->DrawTexturedLine(a, b); 

	g_pActiveSurfaceBuffer->AddToTail( 
		new BufferCallTexturedLine( a, b )
	);

}
void CSurfaceBuffered::DrawOutlinedCircle(int x, int y, int radius, int segments) 
{ 
	// BUFFER CALL
	pRealVGuiSurface->DrawOutlinedCircle(x, y, radius, segments); 

	g_pActiveSurfaceBuffer->AddToTail( 
		new BufferCallArg4<void, int, int, int, int>( &ISurface::DrawOutlinedCircle, x, y, radius, segments )
	);
}
void CSurfaceBuffered::DrawTexturedPolyLine( const Vertex_t *p,int n ) 
{ 
	// BUFFER CALL
	pRealVGuiSurface->DrawTexturedPolyLine( p, n ); 

	g_pActiveSurfaceBuffer->AddToTail( 
		new BufferCallTexturedPolyLine( p, n )
	);
	
} 
void CSurfaceBuffered::DrawTexturedSubRect( int x0, int y0, int x1, int y1, float texs0, float text0, float texs1, float text1 ) 
{ 
	// BUFFER CALL
	pRealVGuiSurface->DrawTexturedSubRect(x0, y0, x1, y1, texs0, text0, texs1, text1); 

	g_pActiveSurfaceBuffer->AddToTail( 
		new BufferCallArg8<void, int, int, int, int, float, float, float, float>( &ISurface::DrawTexturedSubRect, x0, y0, x1, y1, texs0, text0, texs1, text1 )
	);
}
void CSurfaceBuffered::DrawTexturedPolygon(int n, Vertex_t *pVertices, bool bClipVertices ) 
{ 
	// BUFFER CALL
	pRealVGuiSurface->DrawTexturedPolygon( n, pVertices, bClipVertices );

	g_pActiveSurfaceBuffer->AddToTail( 
		new BufferCallTexturedPolygon( n, pVertices, bClipVertices )
	);
}

const wchar_t *CSurfaceBuffered::GetTitle(VPANEL panel) { return pRealVGuiSurface->GetTitle(panel); }
bool CSurfaceBuffered::IsCursorLocked( void ) const { return pRealVGuiSurface->IsCursorLocked(); }
void CSurfaceBuffered::SetWorkspaceInsets( int left, int top, int right, int bottom ) { pRealVGuiSurface->SetWorkspaceInsets(left, top, right, bottom); }

void CSurfaceBuffered::DrawWordBubble( int x0, int y0, int x1, int y1, int nBorderThickness, Color rgbaBackground, Color rgbaBorder, 
								bool bPointer, int nPointerX, int nPointerY, int nPointerBaseThickness )
{
	pRealVGuiSurface->DrawWordBubble( x0, y0, x1, y1, nBorderThickness, rgbaBackground, rgbaBorder, 
		bPointer, nPointerX, nPointerY, nPointerBaseThickness );
}

// Lower level char drawing code, call DrawGet then pass in info to DrawRender
bool CSurfaceBuffered::DrawGetUnicodeCharRenderInfo( wchar_t ch, FontCharRenderInfo& info ) { return pRealVGuiSurface->DrawGetUnicodeCharRenderInfo(ch, info); }
void CSurfaceBuffered::DrawRenderCharFromInfo( const FontCharRenderInfo& info ) { pRealVGuiSurface->DrawRenderCharFromInfo(info); }

// global alpha setting functions
// affect all subsequent draw calls - shouldn't normally be used directly, only in Panel::PaintTraverse()
void CSurfaceBuffered::DrawSetAlphaMultiplier( float alpha /* [0..1] */ ) { pRealVGuiSurface->DrawSetAlphaMultiplier(alpha); }
float CSurfaceBuffered::DrawGetAlphaMultiplier() { return pRealVGuiSurface->DrawGetAlphaMultiplier(); }

// web browser
void CSurfaceBuffered::SetAllowHTMLJavaScript( bool state ) { pRealVGuiSurface->SetAllowHTMLJavaScript(state); }

// video mode changing
void CSurfaceBuffered::OnScreenSizeChanged( int nOldWidth, int nOldHeight ) { pRealVGuiSurface->OnScreenSizeChanged(nOldWidth, nOldHeight); }

vgui::HCursor CSurfaceBuffered::CreateCursorFromFile( char const *curOrAniFile, char const *pPathID ) { return pRealVGuiSurface->CreateCursorFromFile( curOrAniFile, pPathID ); }

// create IVguiMatInfo object ( IMaterial wrapper in VguiMatSurface, NULL in CWin32Surface )
IVguiMatInfo *CSurfaceBuffered::DrawGetTextureMatInfoFactory( int id ) { return pRealVGuiSurface->DrawGetTextureMatInfoFactory(id); }

void CSurfaceBuffered::PaintTraverseEx(VPANEL panel, bool paintPopups ) 
{ 
	Msg("CSurfaceBuffered::PaintTraverseEx\n");
	pRealVGuiSurface->PaintTraverseEx(panel, paintPopups); 
}

float CSurfaceBuffered::GetZPos() const { return pRealVGuiSurface->GetZPos(); }

// From the Xbox
void CSurfaceBuffered::SetPanelForInput( VPANEL vpanel ) { pRealVGuiSurface->SetPanelForInput( vpanel ); }
void CSurfaceBuffered::DrawFilledRectFade( int x0, int y0, int x1, int y1, unsigned int alpha0, unsigned int alpha1, bool bHorizontal ) 
{ 
	// BUFFER CALL
	pRealVGuiSurface->DrawFilledRectFade( x0, y0, x1, y1, alpha0, alpha1, bHorizontal ); 

	g_pActiveSurfaceBuffer->AddToTail( 
		new BufferCallArg7<void, int, int, int, int, unsigned int, unsigned int, bool>( &ISurface::DrawFilledRectFade, x0, y0, x1, y1, alpha0, alpha1, bHorizontal )
	);
	
}
void CSurfaceBuffered::DrawSetTextureRGBAEx(int id, const unsigned char *rgba, int wide, int tall, ImageFormat imageFormat ) 
{ 
	// BUFFER CALL (TODO)
	Msg("pRealVGuiSurface->DrawSetTextureRGBAEx\n");
	pRealVGuiSurface->DrawSetTextureRGBAEx( id, rgba, wide, tall, imageFormat ); 
}

void CSurfaceBuffered::DrawSetTextScale(float sx, float sy) 
{ 
	// BUFFER CALL
	pRealVGuiSurface->DrawSetTextScale( sx, sy );

	g_pActiveSurfaceBuffer->AddToTail( 
		new BufferCallArg2<void, float, float>( &ISurface::DrawSetTextScale, sx, sy )
	);
}
bool CSurfaceBuffered::SetBitmapFontGlyphSet(HFont font, const char *windowsFontName, float scalex, float scaley, int flags) { return pRealVGuiSurface->SetBitmapFontGlyphSet( font, windowsFontName, scalex, scaley, flags ); }
// adds a bitmap font file
bool CSurfaceBuffered::AddBitmapFontFile(const char *fontFileName) { return pRealVGuiSurface->AddBitmapFontFile(fontFileName); }
// sets a symbol for the bitmap font
void CSurfaceBuffered::SetBitmapFontName( const char *pName, const char *pFontFilename ) { pRealVGuiSurface->SetBitmapFontName( pName, pFontFilename ); }
// gets the bitmap font filename
const char *CSurfaceBuffered::GetBitmapFontName( const char *pName ) { return pRealVGuiSurface->GetBitmapFontName(pName); }
void CSurfaceBuffered::ClearTemporaryFontCache( void ) { pRealVGuiSurface->ClearTemporaryFontCache(); }

IImage *CSurfaceBuffered::GetIconImageForFullPath( char const *pFullPath ) { return pRealVGuiSurface->GetIconImageForFullPath(pFullPath); }
void CSurfaceBuffered::DrawUnicodeString( const wchar_t *pwString, FontDrawType_t drawType )
{ 
	// BUFFER CALL
	pRealVGuiSurface->DrawUnicodeString( pwString, drawType ); 

	g_pActiveSurfaceBuffer->AddToTail( 
		new BufferCallUnicodeString( pwString, drawType )
	);
	
}

void CSurfaceBuffered::PrecacheFontCharacters(HFont font, wchar_t *pCharacters) { pRealVGuiSurface->PrecacheFontCharacters(font, pCharacters); }
// Console-only.  Get the string to use for the current video mode for layout files.
const char *CSurfaceBuffered::GetResolutionKey( void ) const { return pRealVGuiSurface->GetResolutionKey(); }

// ASW
const char *CSurfaceBuffered::GetFontName( HFont font ) 
{ 
	return pRealVGuiSurface->GetFontName( font ); 
}

bool CSurfaceBuffered::ForceScreenSizeOverride( bool bState, int wide, int tall ) 
{ 
	return pRealVGuiSurface->ForceScreenSizeOverride( bState, wide, tall );
}

// LocalToScreen, ParentLocalToScreen fixups for explicit PaintTraverse calls on Panels not at 0, 0 position
bool CSurfaceBuffered::ForceScreenPosOffset( bool bState, int x, int y )  
{ 
	return pRealVGuiSurface->ForceScreenPosOffset( bState, x, y );;
}

void CSurfaceBuffered::OffsetAbsPos( int &x, int &y ) 
{
	pRealVGuiSurface->OffsetAbsPos( x, y );
}

void CSurfaceBuffered::SetAbsPosForContext( int id, int x, int y ) 
{
	pRealVGuiSurface->SetAbsPosForContext( id, x, y );
}

void CSurfaceBuffered::GetAbsPosForContext( int id, int &x, int& y ) 
{
	pRealVGuiSurface->GetAbsPosForContext( id, x, y );
}

// Causes fonts to get reloaded, etc.
void CSurfaceBuffered::ResetFontCaches() 
{
	pRealVGuiSurface->ResetFontCaches();
}

bool CSurfaceBuffered::IsScreenSizeOverrideActive( void ) 
{ 
	return pRealVGuiSurface->IsScreenSizeOverrideActive(); 
}

bool CSurfaceBuffered::IsScreenPosOverrideActive( void )
{ 
	return pRealVGuiSurface->IsScreenPosOverrideActive(); 
}

void CSurfaceBuffered::DestroyTextureID( int id ) 
{
	pRealVGuiSurface->DestroyTextureID( id );
}

int CSurfaceBuffered::GetTextureNumFrames( int id ) 
{ 
	return pRealVGuiSurface->GetTextureNumFrames( id );
}

void CSurfaceBuffered::DrawSetTextureFrame( int id, int nFrame, unsigned int *pFrameCache )
{
	pRealVGuiSurface->DrawSetTextureFrame( id, nFrame, pFrameCache );
}

void CSurfaceBuffered::GetClipRect( int &x0, int &y0, int &x1, int &y1 ) 
{
	pRealVGuiSurface->GetClipRect( x0, y0, x1, y1 );
}
void CSurfaceBuffered::SetClipRect( int x0, int y0, int x1, int y1 ) 
{
	pRealVGuiSurface->SetClipRect( x0, y0, x1, y1 );
}

void CSurfaceBuffered::DrawTexturedRectEx( DrawTexturedRectParms_t *pDrawParms ) 
{
	pRealVGuiSurface->DrawTexturedRectEx( pDrawParms );
}