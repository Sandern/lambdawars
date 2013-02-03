//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "src_python_materials.h"

#include "materialsystem/ITexture.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef HL2WARS_ASW_DLL
#ifdef CLIENT_DLL
bool PyIsDeferredRenderingEnabled()
{
	return false; //GetDeferredManager()->IsDeferredRenderingEnabled();
}
#endif // CLIENT_DLL
#endif // HL2WARS_ASW_DLL

#ifdef CLIENT_DLL

static ConVar g_debug_pyproceduralmaterial("g_debug_pyproceduralmaterial", "0", FCVAR_CHEAT);

CUtlVector< PyProceduralTexture *> g_PyProceduralTextures;

class CPyTextureRegen : public ITextureRegenerator
{
public:
	CPyTextureRegen( PyProceduralTexture *pTexture ) : m_pTexture(pTexture) {}
	virtual void RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pSubRect );
	virtual void Release() {}

private:
	PyProceduralTexture *m_pTexture;
};

void CPyTextureRegen::RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pSubRect )
{
	unsigned char *imageData = pVTFTexture->ImageData( 0, 0, 0 );
	if( g_debug_pyproceduralmaterial.GetBool() )
		DevMsg("\tDownloading procedural texture... (debug id: %d)\n", m_pTexture->GetDebugID());
	CUtlVector< char > &newData = m_pTexture->m_TextureData;
	if( !newData.Count() )
		return;
	memcpy( imageData, newData.Base(), newData.Count() );
}

PyProceduralTexture::PyProceduralTexture( const char *pTextureName, const char *pTextureGroupName, int w, int h, ImageFormat fmt, int nFlags )
{
	static int sDebugID = 0;
	m_iDebugID = sDebugID++;

	if( g_debug_pyproceduralmaterial.GetBool() )
		DevMsg("Initializing procedural texture %s (debug id: %d)\n", pTextureName, GetDebugID());

	// Clear any existing procedural textures with the same name
	for( int i = g_PyProceduralTextures.Count()-1; i >= 0; i-- )
	{
		if( Q_strnicmp(g_PyProceduralTextures[i]->GetName(), pTextureName, 256) == 0 )
		{
			g_PyProceduralTextures[i]->Shutdown();
			if( g_debug_pyproceduralmaterial.GetBool() )
				DevMsg("Shutted down duplicate procedural texture %s (debug id: %d)\n", pTextureName, g_PyProceduralTextures[i]->GetDebugID());
		}
	}

	g_PyProceduralTextures.AddToTail( this );

	Q_strncpy( m_sTextureName, pTextureName, 256 );

	m_pTextureRegen = new CPyTextureRegen( this );

	m_RenderBuffer.InitProceduralTexture( pTextureName, TEXTURE_GROUP_CLIENT_EFFECTS, w, h, fmt, 
		TEXTUREFLAGS_PROCEDURAL|TEXTUREFLAGS_SINGLECOPY||TEXTUREFLAGS_NOLOD|TEXTUREFLAGS_NOMIP|nFlags );
	m_RenderBuffer->SetTextureRegenerator( m_pTextureRegen );

	// TODO: Can't query ITexture for this?
	switch( fmt )
	{
	case IMAGE_FORMAT_I8:
		m_iChannels = 1;
		break;
	case IMAGE_FORMAT_RGBA8888:
		m_iChannels = 4;
		break;
	default:
		m_iChannels = 1;
		break;
	}

	// Init our texture data
	m_TextureData.SetCount( m_RenderBuffer->GetActualWidth() * m_RenderBuffer->GetActualHeight() * m_iChannels );

	m_bIsModified = true;
}

PyProceduralTexture::~PyProceduralTexture()
{
	Shutdown();
	g_PyProceduralTextures.FindAndRemove( this );
}

bool PyProceduralTexture::IsValid()
{
	return m_RenderBuffer.IsValid();
}

void PyProceduralTexture::Shutdown()
{
	if( !m_RenderBuffer.IsValid() )
		return;

	if( g_debug_pyproceduralmaterial.GetBool() )
		DevMsg("Shutting down procedural texture %s (debug id: %d)\n", m_sTextureName, GetDebugID());

	m_TextureData.RemoveAll();

	if( m_RenderBuffer.IsValid() )
	{
		m_RenderBuffer->SetTextureRegenerator( NULL );
		m_RenderBuffer.Shutdown();
	}

	if( m_pTextureRegen )
	{
		delete m_pTextureRegen;
		m_pTextureRegen = NULL;
	}
}

void PyProceduralTexture::Download()
{
	m_bIsModified = false;

	if( !IsValid() )
		return;

	if( g_debug_pyproceduralmaterial.GetBool() )
		DevMsg("Python procedural texture %s changed. Downloading... (debug id: %d)\n", m_sTextureName, GetDebugID());
	m_RenderBuffer->Download();
}

void PyProceduralTexture::SetAllPixels( int i )
{
	if( !IsValid() )
		return;
	m_TextureData.FillWithValue( (char)i );
	m_bIsModified = true;
}

void PyProceduralTexture::SetPixel( int x, int y, int i )
{
	if( !IsValid() )
		return;

	if( m_RenderBuffer->GetImageFormat() != IMAGE_FORMAT_I8 )
	{
		PyErr_SetString(PyExc_ValueError, "This SetPixel should only be used with IMAGE_FORMAT_I8.");
		throw boost::python::error_already_set(); 
		return;
	}

	int w = m_RenderBuffer->GetActualWidth();
	int offset = w * y + x;
	if( offset < 0 || offset >= m_TextureData.Count() )
	{
		PyErr_SetString(PyExc_ValueError, "SetPixel overflow");
		throw boost::python::error_already_set(); 
		return;
	}

	m_TextureData[ offset ] = (char)i;
	m_bIsModified = true;
}

void PyProceduralTexture::SetPixel( int x, int y, int r, int g, int b, int a )
{
	if( !IsValid() )
		return;

	if( m_RenderBuffer->GetImageFormat() != IMAGE_FORMAT_RGBA8888 )
	{
		PyErr_SetString(PyExc_ValueError, "This SetPixel should only be used with IMAGE_FORMAT_I8.");
		throw boost::python::error_already_set(); 
		return;
	}

	int w = m_RenderBuffer->GetActualWidth();
	int offset = (w * y * m_iChannels) + (x * m_iChannels);

	if( offset < 0 || offset + 3 >= m_TextureData.Count() )
	{
		PyErr_SetString(PyExc_ValueError, "SetPixel overflow");
		throw boost::python::error_already_set(); 
		return;
	}

	m_TextureData[ offset ] = (char)b;
	m_TextureData[ offset + 1 ] = (char)g;
	m_TextureData[ offset + 2 ] = (char)r;
	m_TextureData[ offset + 3 ] = (char)a;

	m_bIsModified = true;
}

void PyProceduralTexture::SetPixel( int x, int y, const Color &c )
{
	SetPixel( x, y, c.r(), c.g(), c.b(), c.a() );
}

void PyUpdateProceduralMaterials()
{
	for( int i = 0; i < g_PyProceduralTextures.Count(); i++ )
	{
		if( g_PyProceduralTextures[i]->IsModified() )
			g_PyProceduralTextures[i]->Download();
	}
}

void PyShutdownProceduralMaterials()
{
	for( int i = 0; i < g_PyProceduralTextures.Count(); i++ )
	{
		g_PyProceduralTextures[i]->Shutdown();
	}
}

#endif // CLIENT_DLL