//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "fx_envelope.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
C_EnvelopeFX::C_EnvelopeFX( void )
{
	m_active = false;
}

C_EnvelopeFX::~C_EnvelopeFX()
{
	RemoveRenderable();
}

const matrix3x4_t & C_EnvelopeFX::RenderableToWorldTransform()
{
	static matrix3x4_t mat;
	SetIdentityMatrix( mat );
	PositionMatrix( GetRenderOrigin(), mat );
	return mat;
}

void C_EnvelopeFX::RemoveRenderable()
{
	ClientLeafSystem()->RemoveRenderable( m_hRenderHandle );
}

//-----------------------------------------------------------------------------
// Purpose: Updates the envelope being in the client's known entity list
//-----------------------------------------------------------------------------
void C_EnvelopeFX::Update( void )
{
	if ( m_active )
	{
		if ( m_hRenderHandle == INVALID_CLIENT_RENDER_HANDLE )
		{
			ClientLeafSystem()->AddRenderable( this, false, RENDERABLE_IS_TRANSLUCENT, RENDERABLE_MODEL_ENTITY );
		}
		else
		{
			ClientLeafSystem()->RenderableChanged( m_hRenderHandle );
		}
	}
	else
	{
		RemoveRenderable();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Starts up the effect
// Input  : entityIndex - entity to be attached to
//			attachment - attachment point (if any) to be attached to
//-----------------------------------------------------------------------------
void C_EnvelopeFX::EffectInit( int entityIndex, int attachment )
{
	m_entityIndex = entityIndex;
	m_attachment = attachment;

	m_active = 1;
	m_t = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Shuts down the effect
//-----------------------------------------------------------------------------
void C_EnvelopeFX::EffectShutdown( void ) 
{
	m_active = 0;
	m_t = 0;
}

#ifdef ENABLE_PYTHON
//------------------------------------------------------------------------------
// Purpose: Python memory allocation. Same as new.
//------------------------------------------------------------------------------
void *C_EnvelopeFX::PyAllocate(PyObject* self_, std::size_t holder_offset, std::size_t holder_size)
{
	Assert( holder_size != 0 );	
	MEM_ALLOC_CREDIT();
	void *pMem = malloc( holder_size );
	memset( pMem, 0, holder_size );
	return pMem;	
}

void C_EnvelopeFX::PyDeallocate(PyObject* self_, void *storage)
{
#ifdef _DEBUG
	// set the memory to a known value
	int size = g_pMemAlloc->GetSize( storage );
	Q_memset( storage, 0xdd, size );
#endif

	// get the engine to free the memory
	g_pMemAlloc->Free( storage );
}	
#endif // ENABLE_PYTHON