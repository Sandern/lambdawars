#ifndef WEBVIEW_TEXGEN_H
#define WEBVIEW_TEXGEN_H
#ifdef _WIN32
#pragma once
#endif

#include "materialsystem/ITexture.h"
#include <Awesomium/Surface.h>
#include "igamesystem.h"
#include "materialsystem\MaterialSystemUtil.h"

namespace Awesomium {
	class WebView;
	class WebCore;
	class WebSession;
};
class WebView;
class SrcWebViewSurface;
class SrcWebViewSurfaceFactory;

SrcWebViewSurfaceFactory *GetWebViewSurfaceFactory();

void AddWebview( Awesomium::WebView *pView, SrcWebViewSurface *pSurface );
void RemoveWebview( Awesomium::WebView *pView );

void SetAllowTextureGeneration( bool bAllowTextureGeneration );

//-----------------------------------------------------------------------------
// Purpose: WebCore system
//-----------------------------------------------------------------------------
extern Awesomium::WebCore *g_pWebCore;

extern CUtlMap<int, int> m_AwesomiumSrcKeyMap;

class CSrcWebCore : public CAutoGameSystemPerFrame
{
public:
	virtual bool Init();
	virtual void Shutdown();
	virtual void Update( float frametime );

	virtual void LevelInitPreEntity();
	virtual void LevelInitPostEntity();

	Awesomium::WebSession *GetSession();

public:
	bool m_bIngame; 

private:
	Awesomium::WebSession *m_pSession;
};

CSrcWebCore &WebCoreSystem();

//-----------------------------------------------------------------------------
// Purpose: Texture generation
//-----------------------------------------------------------------------------
class CWebViewTextureRegen : public ITextureRegenerator
{
public:
	CWebViewTextureRegen() : m_bRequiresRegeneration(true), m_pSrcBuffer(NULL), m_pImageDataBuffer(NULL), m_bScrollScheduled(false) { }
	virtual void RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pSubRect );
	virtual void Release() {}
	void ScheduleInit() { m_bDoFullInit = true; m_bRequiresRegeneration = true; }
	bool RequiresRegeneration() { return m_bRequiresRegeneration; }

	void SetCopyFromBuffer( unsigned char* src_buffer, int src_row_span, int src_x = 0, int src_y = 0 );
	void SetupScrollArea( int dx, int dy );
	unsigned char *CWebViewTextureRegen::GetImageData();

private:
	bool m_bDoFullInit;
	bool m_bRequiresRegeneration;

	unsigned char* m_pSrcBuffer; 
	int m_iSrcRowPan, m_iSrcX, m_iSrcY;

	bool m_bScrollScheduled;
	int m_iScrolldx, m_iScrolldy;

	unsigned char *m_pImageDataBuffer;
	int m_iBufferWidth, m_iBufferHeight;
};

inline void CWebViewTextureRegen::SetCopyFromBuffer( unsigned char* src_buffer, int src_row_span, int src_x, int src_y ) 
{
	m_pSrcBuffer = src_buffer;
	m_iSrcRowPan = src_row_span;
	m_iSrcX = src_x; 
	m_iSrcY = src_y;
}

inline void CWebViewTextureRegen::SetupScrollArea( int dx, int dy )
{
	m_bScrollScheduled = true;
	m_iScrolldx = dx;
	m_iScrolldy = dy;
}

inline unsigned char *CWebViewTextureRegen::GetImageData() 
{ 
	return m_pImageDataBuffer; 
}

//-----------------------------------------------------------------------------
// Purpose: Surface
//-----------------------------------------------------------------------------
class SrcWebViewSurface : public Awesomium::Surface
{
public:
	friend class CWebViewTextureRegen;

	SrcWebViewSurface();
	~SrcWebViewSurface();

	void Clear();

	void Paint(unsigned char* src_buffer,
			int src_row_span,
			const Awesomium::Rect& src_rect,
			const Awesomium::Rect& dest_rect);
	void Scroll(int dx, int dy,
			const Awesomium::Rect& clip_rect);

	void Resize( int width, int height ); // Resize procedural texture if needed
	void Draw( int width, int tall ); // Draws the texture

	int GetWidth();
	int GetTall();
	int GetChannels();

	int GetClipWidth();
	int GetClipTall();

	unsigned char *GetImageData();

private:
	CTextureReference	m_RenderBuffer;
	CWebViewTextureRegen *m_pWebViewTextureRegen;
	CMaterialReference m_MatRef;
	char m_MatWebViewName[MAX_PATH];
	char m_TextureWebViewName[MAX_PATH];

	int m_iTextureID;
	int m_iWVWide, m_iWVTall;
	int m_iTexWide, m_iTexTall;
	float m_fTexS1, m_fTexT1;
	Color m_Color;
	bool m_bSizeChanged;
};

inline int SrcWebViewSurface::GetWidth() 
{ 
	return m_iTexWide; 
}

inline int SrcWebViewSurface::GetTall() 
{ 
	return m_iTexTall; 
}

inline int SrcWebViewSurface::GetChannels() 
{ 
	return 4; 
}

inline int SrcWebViewSurface::GetClipWidth() 
{ 
	return m_iWVWide; 
}

inline int SrcWebViewSurface::GetClipTall() 
{ 
	return m_iWVTall; 
}

inline unsigned char *SrcWebViewSurface::GetImageData() 
{ 
	return m_pWebViewTextureRegen->GetImageData(); 
}

//-----------------------------------------------------------------------------
// Purpose: Surface factory
//-----------------------------------------------------------------------------
class SrcWebViewSurfaceFactory : public Awesomium::SurfaceFactory 
{
public:
	Awesomium::Surface* CreateSurface(Awesomium::WebView* view, int width, int height);
	void DestroySurface(Awesomium::Surface* surface);
};

#endif // WEBVIEW_TEXGEN_H