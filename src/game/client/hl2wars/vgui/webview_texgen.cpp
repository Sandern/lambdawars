#include "cbase.h"
#include "webview_texgen.h"
#include <vgui_controls/Controls.h>
#include <vgui/ISurface.h>
#include <utlmap.h>

#include <Awesomium/WebCore.h>

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

ConVar g_webview_draw("g_webview_draw", "1", FCVAR_CHEAT);
ConVar g_debug_webview("g_debug_webview", "0", FCVAR_CHEAT);

bool g_AllowTextureGeneration = true;

//#define DISABLE_AWESOMIUM

Awesomium::WebCore *g_pWebCore = NULL;

//-----------------------------------------------------------------------------
// Purpose: Surface factory
//-----------------------------------------------------------------------------
static SrcWebViewSurfaceFactory s_SrcWebViewSurfaceFactory;

SrcWebViewSurfaceFactory *GetWebViewSurfaceFactory()
{
	return &s_SrcWebViewSurfaceFactory;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void SetAllowTextureGeneration( bool bAllowTextureGeneration )
{
	//g_AllowTextureGeneration = bAllowTextureGeneration;
}

//-----------------------------------------------------------------------------
// Purpose: Helper functions for finding back the surface for a webview
//-----------------------------------------------------------------------------
struct SurfaceEntry
{
	Awesomium::WebView *webview;
	SrcWebViewSurface *surface;
};

static CUtlVector< SurfaceEntry > g_Surfaces;

void AddWebview( Awesomium::WebView *pView, SrcWebViewSurface *pSurface )
{
	if( !pView )
		return;

	g_Surfaces.AddToTail();
	int i = g_Surfaces.Count() - 1;
	g_Surfaces[i].webview = pView;
	g_Surfaces[i].surface = pSurface;
}

void RemoveWebview( Awesomium::WebView *pView )
{
	if( !pView )
		return;

	for( int i = 0; i < g_Surfaces.Count(); i++ )
	{
		if( g_Surfaces[i].webview == pView )
		{
			g_Surfaces.Remove( i );
			break;
		}
	}
}

SrcWebViewSurface *FindSurface( Awesomium::WebView *pView )
{
	int i;
	for( i = 0; i < g_Surfaces.Count(); i++ )
	{
		if( g_Surfaces[i].webview == pView )
			return g_Surfaces[i].surface;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Webcore system
//-----------------------------------------------------------------------------
bool AweSrcKeyMapLessFunc( const int &lhs, const int &rhs )	
{ 
	return ( lhs > rhs );	
}

CUtlMap<int, int> m_AwesomiumSrcKeyMap;

static void AddOption( Awesomium::WebConfig &config, const char *pOption )
{
	Awesomium::WebString woption = Awesomium::WebString::CreateFromUTF8(pOption, Q_strlen(pOption));
	config.additional_options.Push( woption );
}

bool CSrcWebCore::Init() 
{
	SetAllowTextureGeneration( false );

#ifndef DISABLE_AWESOMIUM
	Awesomium::WebConfig config;
	config.log_level = Awesomium::kLogLevel_Verbose;

	AddOption( config, "--disable-fullscreen=1" );
	

	// Create the WebCore singleton with our custom config
	g_pWebCore = Awesomium::WebCore::Initialize( config );

	// Custom surface factory
	g_pWebCore->set_surface_factory( GetWebViewSurfaceFactory() );

	// Create session
	Awesomium::WebPreferences preferences;
	//preferences.allow_file_access_from_file_url = true;
	//preferences.allow_universal_access_from_file_url = true;
	preferences.enable_web_security = false;

	Awesomium::WebString sessionpath = Awesomium::WebString::CreateFromUTF8("session.dat", Q_strlen("session.dat"));
	m_pSession = g_pWebCore->CreateWebSession( sessionpath, preferences );

	// Initialize Key Map
	m_AwesomiumSrcKeyMap.SetLessFunc( AweSrcKeyMapLessFunc );

	m_AwesomiumSrcKeyMap.Insert( KEY_0, Awesomium::KeyCodes::AK_0 );
	m_AwesomiumSrcKeyMap.Insert( KEY_1, Awesomium::KeyCodes::AK_1 );
	m_AwesomiumSrcKeyMap.Insert( KEY_2, Awesomium::KeyCodes::AK_2 );
	m_AwesomiumSrcKeyMap.Insert( KEY_3, Awesomium::KeyCodes::AK_3 );
	m_AwesomiumSrcKeyMap.Insert( KEY_4, Awesomium::KeyCodes::AK_4 );
	m_AwesomiumSrcKeyMap.Insert( KEY_5, Awesomium::KeyCodes::AK_5 );
	m_AwesomiumSrcKeyMap.Insert( KEY_6, Awesomium::KeyCodes::AK_6 );
	m_AwesomiumSrcKeyMap.Insert( KEY_7, Awesomium::KeyCodes::AK_7 );
	m_AwesomiumSrcKeyMap.Insert( KEY_8, Awesomium::KeyCodes::AK_8 );
	m_AwesomiumSrcKeyMap.Insert( KEY_9, Awesomium::KeyCodes::AK_9 );

	m_AwesomiumSrcKeyMap.Insert( KEY_A, Awesomium::KeyCodes::AK_A );
	m_AwesomiumSrcKeyMap.Insert( KEY_B, Awesomium::KeyCodes::AK_B );
	m_AwesomiumSrcKeyMap.Insert( KEY_C, Awesomium::KeyCodes::AK_C );
	m_AwesomiumSrcKeyMap.Insert( KEY_D, Awesomium::KeyCodes::AK_D );
	m_AwesomiumSrcKeyMap.Insert( KEY_E, Awesomium::KeyCodes::AK_E );
	m_AwesomiumSrcKeyMap.Insert( KEY_F, Awesomium::KeyCodes::AK_F );
	m_AwesomiumSrcKeyMap.Insert( KEY_G, Awesomium::KeyCodes::AK_G );
	m_AwesomiumSrcKeyMap.Insert( KEY_H, Awesomium::KeyCodes::AK_H );
	m_AwesomiumSrcKeyMap.Insert( KEY_I, Awesomium::KeyCodes::AK_I );
	m_AwesomiumSrcKeyMap.Insert( KEY_J, Awesomium::KeyCodes::AK_J );
	m_AwesomiumSrcKeyMap.Insert( KEY_K, Awesomium::KeyCodes::AK_K );
	m_AwesomiumSrcKeyMap.Insert( KEY_L, Awesomium::KeyCodes::AK_L );
	m_AwesomiumSrcKeyMap.Insert( KEY_M, Awesomium::KeyCodes::AK_M );
	m_AwesomiumSrcKeyMap.Insert( KEY_N, Awesomium::KeyCodes::AK_N );
	m_AwesomiumSrcKeyMap.Insert( KEY_O, Awesomium::KeyCodes::AK_O );
	m_AwesomiumSrcKeyMap.Insert( KEY_P, Awesomium::KeyCodes::AK_P );
	m_AwesomiumSrcKeyMap.Insert( KEY_Q, Awesomium::KeyCodes::AK_Q );
	m_AwesomiumSrcKeyMap.Insert( KEY_R, Awesomium::KeyCodes::AK_R );
	m_AwesomiumSrcKeyMap.Insert( KEY_S, Awesomium::KeyCodes::AK_S );
	m_AwesomiumSrcKeyMap.Insert( KEY_T, Awesomium::KeyCodes::AK_T );
	m_AwesomiumSrcKeyMap.Insert( KEY_U, Awesomium::KeyCodes::AK_U );
	m_AwesomiumSrcKeyMap.Insert( KEY_V, Awesomium::KeyCodes::AK_V );
	m_AwesomiumSrcKeyMap.Insert( KEY_W, Awesomium::KeyCodes::AK_W );
	m_AwesomiumSrcKeyMap.Insert( KEY_X, Awesomium::KeyCodes::AK_X );
	m_AwesomiumSrcKeyMap.Insert( KEY_Y, Awesomium::KeyCodes::AK_Y );
	m_AwesomiumSrcKeyMap.Insert( KEY_Z, Awesomium::KeyCodes::AK_Z );

	m_AwesomiumSrcKeyMap.Insert( KEY_PAD_0, Awesomium::KeyCodes::AK_NUMPAD0 );
	m_AwesomiumSrcKeyMap.Insert( KEY_PAD_1, Awesomium::KeyCodes::AK_NUMPAD1 );
	m_AwesomiumSrcKeyMap.Insert( KEY_PAD_2, Awesomium::KeyCodes::AK_NUMPAD2 );
	m_AwesomiumSrcKeyMap.Insert( KEY_PAD_3, Awesomium::KeyCodes::AK_NUMPAD3 );
	m_AwesomiumSrcKeyMap.Insert( KEY_PAD_4, Awesomium::KeyCodes::AK_NUMPAD4 );
	m_AwesomiumSrcKeyMap.Insert( KEY_PAD_5, Awesomium::KeyCodes::AK_NUMPAD5 );
	m_AwesomiumSrcKeyMap.Insert( KEY_PAD_6, Awesomium::KeyCodes::AK_NUMPAD6 );
	m_AwesomiumSrcKeyMap.Insert( KEY_PAD_7, Awesomium::KeyCodes::AK_NUMPAD7 );
	m_AwesomiumSrcKeyMap.Insert( KEY_PAD_8, Awesomium::KeyCodes::AK_NUMPAD8 );
	m_AwesomiumSrcKeyMap.Insert( KEY_PAD_9, Awesomium::KeyCodes::AK_NUMPAD9 );

	m_AwesomiumSrcKeyMap.Insert( KEY_PAD_DIVIDE, Awesomium::KeyCodes::AK_DIVIDE );
	m_AwesomiumSrcKeyMap.Insert( KEY_PAD_MULTIPLY, Awesomium::KeyCodes::AK_MULTIPLY );
	m_AwesomiumSrcKeyMap.Insert( KEY_PAD_MINUS, Awesomium::KeyCodes::AK_SUBTRACT );
	m_AwesomiumSrcKeyMap.Insert( KEY_PAD_PLUS, Awesomium::KeyCodes::AK_ADD );
	m_AwesomiumSrcKeyMap.Insert( KEY_PAD_ENTER, Awesomium::KeyCodes::AK_RETURN );
	m_AwesomiumSrcKeyMap.Insert( KEY_PAD_DECIMAL, Awesomium::KeyCodes::AK_SEPARATOR );

	// TODO: Double check the oem keys
	m_AwesomiumSrcKeyMap.Insert( KEY_LBRACKET, Awesomium::KeyCodes::AK_OEM_4 ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_RBRACKET, Awesomium::KeyCodes::AK_OEM_6 ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_SEMICOLON, Awesomium::KeyCodes::AK_OEM_1 ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_APOSTROPHE, Awesomium::KeyCodes::AK_OEM_7 ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_BACKQUOTE, Awesomium::KeyCodes::AK_OEM_3 ); 

	m_AwesomiumSrcKeyMap.Insert( KEY_COMMA, Awesomium::KeyCodes::AK_OEM_COMMA ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_PERIOD, Awesomium::KeyCodes::AK_OEM_PERIOD ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_COMMA, Awesomium::KeyCodes::AK_OEM_COMMA ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_SLASH, Awesomium::KeyCodes::AK_OEM_2 ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_BACKSLASH, Awesomium::KeyCodes::AK_OEM_5 ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_MINUS, Awesomium::KeyCodes::AK_OEM_MINUS ); 
	//m_AwesomiumSrcKeyMap.Insert( KEY_EQUAL,  );  // TODO

	m_AwesomiumSrcKeyMap.Insert( KEY_ENTER, Awesomium::KeyCodes::AK_RETURN ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_SPACE, Awesomium::KeyCodes::AK_SPACE ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_BACKSPACE, Awesomium::KeyCodes::AK_BACK ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_TAB, Awesomium::KeyCodes::AK_TAB ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_CAPSLOCK, Awesomium::KeyCodes::AK_CAPITAL ); 

	m_AwesomiumSrcKeyMap.Insert( KEY_NUMLOCK, Awesomium::KeyCodes::AK_NUMLOCK ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_ESCAPE, Awesomium::KeyCodes::AK_ESCAPE ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_SCROLLLOCK, Awesomium::KeyCodes::AK_SCROLL ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_INSERT, Awesomium::KeyCodes::AK_INSERT ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_DELETE, Awesomium::KeyCodes::AK_DELETE ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_HOME, Awesomium::KeyCodes::AK_HOME ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_END, Awesomium::KeyCodes::AK_END ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_PAGEUP, Awesomium::KeyCodes::AK_PRIOR ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_PAGEDOWN, Awesomium::KeyCodes::AK_NEXT ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_BREAK, Awesomium::KeyCodes::AK_PAUSE ); 

	m_AwesomiumSrcKeyMap.Insert( KEY_LSHIFT, Awesomium::KeyCodes::AK_LSHIFT ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_RSHIFT, Awesomium::KeyCodes::AK_RSHIFT ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_LALT, Awesomium::KeyCodes::AK_MENU );  // TODO/check
	m_AwesomiumSrcKeyMap.Insert( KEY_RALT, Awesomium::KeyCodes::AK_MENU );  // TODO/check
	m_AwesomiumSrcKeyMap.Insert( KEY_LCONTROL, Awesomium::KeyCodes::AK_LCONTROL ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_RCONTROL, Awesomium::KeyCodes::AK_RCONTROL ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_LWIN, Awesomium::KeyCodes::AK_LWIN ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_RWIN, Awesomium::KeyCodes::AK_RWIN ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_APP, Awesomium::KeyCodes::AK_APPS ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_UP, Awesomium::KeyCodes::AK_UP ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_LEFT, Awesomium::KeyCodes::AK_LEFT ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_DOWN, Awesomium::KeyCodes::AK_DOWN ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_RIGHT, Awesomium::KeyCodes::AK_RIGHT ); 

	m_AwesomiumSrcKeyMap.Insert( KEY_F1, Awesomium::KeyCodes::AK_F1 ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_F2, Awesomium::KeyCodes::AK_F2 ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_F3, Awesomium::KeyCodes::AK_F3 ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_F4, Awesomium::KeyCodes::AK_F4 ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_F5, Awesomium::KeyCodes::AK_F5 ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_F6, Awesomium::KeyCodes::AK_F6 ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_F7, Awesomium::KeyCodes::AK_F7 ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_F8, Awesomium::KeyCodes::AK_F8 ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_F9, Awesomium::KeyCodes::AK_F9 ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_F10, Awesomium::KeyCodes::AK_F10 ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_F11, Awesomium::KeyCodes::AK_F11 ); 
	m_AwesomiumSrcKeyMap.Insert( KEY_F12, Awesomium::KeyCodes::AK_F12 ); 
#endif // DISABLE_AWESOMIUM

	return true; 
}

void CSrcWebCore::Shutdown() 
{
	for( int i = 0; i < g_Surfaces.Count(); i++ )
		g_Surfaces[i].surface->Clear(); // Clear textures

	if( g_pWebCore )
	{
		m_pSession->Release();

		g_pWebCore->set_surface_factory( NULL );
		g_pWebCore->Update();
		Awesomium::WebCore::Shutdown();
		g_pWebCore = NULL;
	}
}

void CSrcWebCore::Update( float frametime ) 
{ 
	if( g_pWebCore )
	{
		try {
			g_pWebCore->Update();
		} catch( ... ) {
			Warning("WebCore throwed an exception!\n");
		}
	}
}

void CSrcWebCore::LevelInitPreEntity() 
{
	SetAllowTextureGeneration( false );
}

void CSrcWebCore::LevelInitPostEntity() 
{
	SetAllowTextureGeneration( true );
}

Awesomium::WebSession *CSrcWebCore::GetSession()
{
	return m_pSession;
}

static CSrcWebCore s_WebCoreSystem;

CSrcWebCore &WebCoreSystem()
{
	return s_WebCoreSystem;
}

//-----------------------------------------------------------------------------
// Purpose: Texture generator
//-----------------------------------------------------------------------------
void CWebViewTextureRegen::RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pSubRect )
{
	int width, height, channels;

	if( engine->IsDrawingLoadingImage() || !g_AllowTextureGeneration )
	{
		if( g_debug_webview.GetBool() )
		{
			if( engine->IsDrawingLoadingImage() )
				DevMsg("WebView: Regenerating image, but denied because of load screen\n");
			if( !g_AllowTextureGeneration )
				DevMsg("WebView: Regenerating image, but denied because of other reasons\n");
		}
		//m_bDoFullInit = true;
		m_bRequiresRegeneration = true;

		m_pSrcBuffer = NULL;
		m_bScrollScheduled = false;
		return;
	}
	
	width = pVTFTexture->Width();
	height = pVTFTexture->Height();
	Assert( pVTFTexture->Format() == IMAGE_FORMAT_BGRA8888 );
	channels = 4; 
	
	unsigned char *imageData = pVTFTexture->ImageData( 0, 0, 0 );

	if( m_bDoFullInit )
	{
		if( g_debug_webview.GetBool() )
			DevMsg("WebView: Initializing texture image data\n");
		// Set the whole texture to zero one time so we don't see the purple checker box.
		memset( imageData, 0, width*height*channels );
		m_bDoFullInit = false;
		m_bRequiresRegeneration = true;

		m_pSrcBuffer = NULL;
		m_bScrollScheduled = false;
		return;
	}

	if( !pSubRect )
	{
		Warning("WebView: Regenerating image, but no area specified\n");
		m_pSrcBuffer = NULL;
		m_bScrollScheduled = false;
		return;
	}

	if( !m_bScrollScheduled && !m_pSrcBuffer )
	{
		if( g_debug_webview.GetBool() )
			DevMsg("WebView: Regenerating image, but no source image buffer set and no scroll action scheduled\n");

		if( m_pImageDataBuffer && m_iBufferWidth == width && m_iBufferHeight == height )
		{
			for( int y = pSubRect->y; y <  pSubRect->y+pSubRect->height; y++ )
			{
				memcpy( imageData + (y * width * channels) + (pSubRect->x * channels), // Our current row + x offset
					m_pImageDataBuffer + (y * width * channels) + (pSubRect->x * channels), // Source offset
					pSubRect->width * channels // size of row we want to copy
				); 
			}

			m_bRequiresRegeneration = false;
		}
		else
		{
			for( int y = pSubRect->y; y <  pSubRect->y+pSubRect->height; y++ )
			{
				memset( imageData + (y * width * channels) + (pSubRect->x * channels), // Our current row + x offset
					0,
					pSubRect->width * channels // size of row we want to copy
				); 
			}
		}
		return;
	}

	if( m_bScrollScheduled )
	{
		if( g_debug_webview.GetBool() )
			DevMsg("WebView: Updating texture (scroll action)\n");

		int y, ystart, yend;
		int maxrowcopy, xdestoffset, xsrcoffset;

		if( !m_pImageDataBuffer )
		{
			m_bScrollScheduled = false;
			return;
		}

		// Calculate horizontal scrolling offsets
		maxrowcopy = (pSubRect->width * channels) - (abs(m_iScrolldx) * channels);
		if( m_iScrolldx > 0 )
		{
			xdestoffset = m_iScrolldx * channels;
			xsrcoffset = 0;
		}
		else if( m_iScrolldx < 0 )
		{
			xdestoffset = 0;
			xsrcoffset = abs(m_iScrolldx) * channels;
		}
		else
		{
			xdestoffset = xsrcoffset = 0;
		}

		// Perform vertical scrolling
		// Negative Y is scrolling down a page, positive Y is up.
		// ystart and yend cover the destination rows
		if( m_iScrolldy < 0 )
		{
			ystart = pSubRect->y;
			yend = pSubRect->y + pSubRect->height;

			for( y = ystart; y < yend; y++ )
			{
				memmove( imageData + (y * width * channels) + (pSubRect->x * channels) + xdestoffset, // Our current row + x offset
						 m_pImageDataBuffer + ((y-m_iScrolldy) * width * channels) + (pSubRect->x * channels) + xsrcoffset, // Row we are moving
						 maxrowcopy // size of row we want to copy
				);
			}
		}
		else if ( m_iScrolldy > 0 )
		{
			ystart = pSubRect->y + pSubRect->height -1;
			yend = pSubRect->y;

			for( int y = ystart; y >= yend; y-- )
			{
				memmove( imageData + (y * width * channels) + (pSubRect->x * channels) + xdestoffset, // Our current row + x offset
						 m_pImageDataBuffer + ((y-m_iScrolldy) * width * channels) + (pSubRect->x * channels) + xsrcoffset, // Row we are moving
						 maxrowcopy // size of row we want to copy
				);
			}
		}
		else
		{
			// Horizontal scrolling only
			ystart = pSubRect->y;
			yend = pSubRect->y + pSubRect->height;

			for( y = ystart; y < yend; y++ )
			{
				memmove( imageData + (y * width * channels) + (pSubRect->x * channels) + xdestoffset, // Our current row + x offset
						 m_pImageDataBuffer + (y * width * channels) + (pSubRect->x * channels) + xsrcoffset, // Row we are moving
						 maxrowcopy // size of row we want to copy
				);
			}		
		}
	}
	else
	{
		if( g_debug_webview.GetBool() )
			DevMsg("WebView: Updating texture (dirty rect)\n");

		// NOTE: code assumes that if pSubRect is not the texture size that it falls in bounds
		//		 of the render texture (which is potential smaller than the texture itself)
		if( width == pSubRect->width && height == pSubRect->height )
		{
			// Width of texture is the same as the render buffer width, so we can just do a memcpy
			// on the whole render buffer
			memcpy( imageData, m_pSrcBuffer, pSubRect->width*pSubRect->height*channels );
		}
		else /*if( pSubRect->height != height || pSubRect->width != width )*/
		{
			// Copy dirty part per line
			// NOTE: m_pSrcBuffer only contains the dirty part and contains pSubRect->width rows.
			//		 pSubRect->y/x are the destination offsets of our full image data buffer.
			for( int y = pSubRect->y; y <  pSubRect->y+pSubRect->height; y++ )
			{
				memcpy( imageData + (y * width * channels) + (pSubRect->x * channels), // Our current row + x offset
					m_pSrcBuffer + ((y - pSubRect->y + m_iSrcY) * m_iSrcRowPan) + (m_iSrcX * channels), // Source offset
					pSubRect->width * channels // size of row we want to copy
				); 
			}
		}
	#if 0
		else
		{
			// Copy per line
			for( int y = 0; y < pSubRect->height; y++ )
			{
				memcpy( imageData + (y * width * channels) + (pSubRect->x * channels), 
					m_pSrcBuffer + y * pSubRect->width * channels, 
					pSubRect->width * channels );
			}
		}
	#endif // 0
	}

	// Clear after using
	m_pSrcBuffer = NULL;
	m_bScrollScheduled = false;

	// Resize buffer if needed
	if( !m_pImageDataBuffer )
	{
		m_pImageDataBuffer = (unsigned char *)malloc( width * height * channels );
		m_iBufferWidth = width;
		m_iBufferHeight = height;
	}
	else if( m_iBufferWidth != width || m_iBufferHeight != height )
	{
		free( m_pImageDataBuffer );
		m_pImageDataBuffer = (unsigned char *)malloc( width * height * channels );
		m_iBufferWidth = width;
		m_iBufferHeight = height;
	}

	// Keep track of changes (needed for scrolling)
	// The reason we need this kind of sucks. imageData doesn't seems to be perfectly the same as the previous state when doing a new regeneration of an area,
	// so we can't rely on that when scrolling.
	for( int y = pSubRect->y; y <  pSubRect->y+pSubRect->height; y++ )
	{
		memcpy( m_pImageDataBuffer + (y * width * channels) + (pSubRect->x * channels), // Our current row + x offset
			imageData + (y * width * channels) + (pSubRect->x * channels), // Source offset
			pSubRect->width * channels // size of row we want to copy
		); 
	}
}

//-----------------------------------------------------------------------------
// Purpose: Surface/Painting
//-----------------------------------------------------------------------------
int nexthigher( int k ) 
{
	k--;
	for (int i=1; i<sizeof(int)*CHAR_BIT; i<<=1)
			k = k | k >> i;
	return k+1;
}

SrcWebViewSurface::SrcWebViewSurface() : m_iTextureID(-1), m_bSizeChanged(false), m_pWebViewTextureRegen(NULL)
{
	m_Color = Color(255, 255, 255, 255);
	m_iTexWide = m_iTexTall = 0;

	static int staticMatWebViewID = 0;
	Q_snprintf( m_MatWebViewName, _MAX_PATH, "vgui/webview/webview_test%d", staticMatWebViewID++ );
}

SrcWebViewSurface::~SrcWebViewSurface()
{
	Clear();
}

void SrcWebViewSurface::Clear()
{
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
	if( m_pWebViewTextureRegen )
	{
		delete m_pWebViewTextureRegen; 
		m_pWebViewTextureRegen = NULL;
	}
}

void SrcWebViewSurface::Resize( int width, int height )
{
	m_iWVWide = width;
	m_iWVTall = height;

	int po2wide = nexthigher(m_iWVWide);
	int po2tall = nexthigher(m_iWVTall);

	if( g_debug_webview.GetBool() )
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

	m_bSizeChanged = true;

	m_iTexWide = po2wide;
	m_iTexTall = po2tall;
	m_fTexS1 = 1.0 - (m_iTexWide - m_iWVWide) / (float)m_iTexWide;
	m_fTexT1 = 1.0 - (m_iTexTall - m_iWVTall) / (float)m_iTexTall;

	if( !m_pWebViewTextureRegen )
	{
		if( g_debug_webview.GetBool() )
			DevMsg("WebView: initializing texture regenerator\n");
		m_pWebViewTextureRegen = new CWebViewTextureRegen();
	}
	else
	{
		if( g_debug_webview.GetBool() )
			DevMsg("WebView: Invalidating old render texture\n");
		m_RenderBuffer->SetTextureRegenerator( NULL );
		m_RenderBuffer.Shutdown();
		m_MatRef.Shutdown();
	}

	static int staticTextureID = 0;
	Q_snprintf( m_TextureWebViewName, _MAX_PATH, "_rt_test%d", staticTextureID++ );

	if( g_debug_webview.GetBool() )
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

	m_pWebViewTextureRegen->ScheduleInit();
	m_RenderBuffer->SetTextureRegenerator( m_pWebViewTextureRegen );
	m_RenderBuffer->Download( NULL );	

	//if( m_MatRef.IsValid() )
	//	m_MatRef.Shutdown();

	if( g_debug_webview.GetBool() )
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

	if( g_debug_webview.GetBool() )
		DevMsg("done.\n");
}

void SrcWebViewSurface::Paint(unsigned char* src_buffer,
		int src_row_span,
		const Awesomium::Rect& src_rect,
		const Awesomium::Rect& dest_rect)
{
	if( !m_pWebViewTextureRegen || !m_RenderBuffer.IsValid() )
		return;

	if( g_debug_webview.GetInt() > 3 )
		DevMsg("WebView: texture requires partial regeneration: src: %d %d %d %d dest: %d %d %d %d src_row_span: %d\n", 
				src_rect.x, src_rect.y, src_rect.width, src_rect.height,
				dest_rect.x, dest_rect.y, dest_rect.width, dest_rect.height, 
				src_row_span );

	Rect_t rect;
	rect.x = dest_rect.x;
	rect.y = dest_rect.y;
	rect.width = dest_rect.width;
	rect.height = dest_rect.height;
	m_RenderBuffer->SetTextureRegenerator( m_pWebViewTextureRegen );

	m_pWebViewTextureRegen->SetCopyFromBuffer( src_buffer, src_row_span, src_rect.x, src_rect.y );
	m_RenderBuffer->Download( &rect );
}

void SrcWebViewSurface::Scroll(int dx, int dy,
		const Awesomium::Rect& clip_rect)
{
	if( !m_pWebViewTextureRegen || !m_RenderBuffer.IsValid() )
		return;

	if( g_debug_webview.GetInt() > 3 )
		DevMsg("WebView: texture requires scrolling: clip: %d %d %d %d dx: %d dy: %d\n", 
				clip_rect.x, clip_rect.y, clip_rect.width, clip_rect.height,
				dx, dy );

	if( abs(dx) > clip_rect.width || abs(dy) >= clip_rect.height )
	{
		if( g_debug_webview.GetInt() > 3 )
			DevMsg("WebView: scrolls exceed page, nothing to scroll.\n");
		return;
	}

	Rect_t rect;
	rect.x = clip_rect.x;
	rect.y = clip_rect.y + ((dy > 0) ? dy : 0);
	rect.width = clip_rect.width;
	rect.height = clip_rect.height + ((dy < 0) ? dy : 0) - ((dy > 0) ? dy : 0);
	if( g_debug_webview.GetInt() > 3 )
		DevMsg("WebView calculated scroll destination area: %d %d %d %d (%d)\n", rect.x, rect.y, rect.width, rect.height, clip_rect.height + ((dy < 0) ? dy : 0) );

	m_pWebViewTextureRegen->SetupScrollArea( dx, dy );
	m_RenderBuffer->Download( &rect );	
}

void SrcWebViewSurface::Draw( int width, int tall )
{
	if( m_bSizeChanged )
	{
		if( g_debug_webview.GetBool() )
			DevMsg("WebView: initializing texture id...");

		m_iTextureID = vgui::surface()->CreateNewTextureID( true );
		vgui::surface()->DrawSetTextureFile( m_iTextureID, m_MatWebViewName, false, false );
		m_bSizeChanged = false;

		if( g_debug_webview.GetBool() )
			DevMsg("done.\n");
	}

	if( m_pWebViewTextureRegen )
	{
		if(  m_pWebViewTextureRegen->RequiresRegeneration() )
		{
			if( g_debug_webview.GetBool() )
				DevMsg("WebView: texture requires full regeneration\n");
			m_RenderBuffer->SetTextureRegenerator( m_pWebViewTextureRegen );
			m_RenderBuffer->Download( NULL );	

			if( m_pWebViewTextureRegen->RequiresRegeneration() == false )
				materials->ReloadMaterials(m_MatWebViewName); // FIXME
		}

		// Must have a valid texture and the texture regenerator should be valid
		if( m_iTextureID != -1 && m_pWebViewTextureRegen->RequiresRegeneration() == false && g_webview_draw.GetBool() )
		{
			vgui::surface()->DrawSetColor( m_Color );
			vgui::surface()->DrawSetTexture( m_iTextureID );
			vgui::surface()->DrawTexturedSubRect( 0, 0, width, tall, 0, 0, m_fTexS1, m_fTexT1 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Surface factory
//-----------------------------------------------------------------------------
Awesomium::Surface* SrcWebViewSurfaceFactory::CreateSurface(Awesomium::WebView* view, int width, int height)
{
	SrcWebViewSurface *pSurface = FindSurface( view );
	if( !pSurface )
	{
		Warning("SrcWebViewSurfaceFactory::CreateSurface: webview without surface!\n");
		return NULL;
	}
	pSurface->Resize( width, height );
	return pSurface;
}

void SrcWebViewSurfaceFactory::DestroySurface(Awesomium::Surface* surface)
{
}