//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "cbase.h"
#include "srcpy.h"
#include "srcpy_hud.h"
#include "hudelement.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Start with empty list
CPyHudElementHelper *CPyHudElementHelper::m_sHelpers = NULL;

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CPyHudElementHelper::CPyHudElementHelper( boost::python::object hud )
{
	// Check valid
	m_pHud = NULL;
	try
	{
		m_pHud = boost::python::extract<CHudElement *>(hud);
	}
	catch(boost::python::error_already_set &)
	{
		PyErr_SetString(PyExc_ValueError, "Invalid hud element. Instance must be of type CHudElement\n" );
		throw boost::python::error_already_set(); 
		return;
	}
	
	// Check for NULL
	if( !m_pHud )
	{	
		PyErr_SetString(PyExc_ValueError, "Invalid hud element. Instance must be of type CHudElement\n" );
		throw boost::python::error_already_set(); 
		return;
	}

	// Add to the global HUD thing
	m_pyHud = hud;
	m_pHud->m_pyInstance = m_pyHud;
	GetHud().AddHudElement( m_pHud );

	// Insert at front
	m_pNext			= m_sHelpers;
	m_pPrev			= NULL;
	if( m_pNext )
		m_pNext->m_pPrev = this;
	m_sHelpers		= this;
}

CPyHudElementHelper::~CPyHudElementHelper(  )
{
	if( m_pHud )
	{
		// Remove hud element
		if( SrcPySystem()->IsPythonRunning() )
		{
			GetHud( m_pHud->GetSplitScreenPlayerSlot() ).RemoveHudElement (m_pHud );
			m_pHud->SetNeedsRemove( false );
		}
		m_pHud->m_pyInstance = boost::python::object();		// Avoid circular reference
		m_pHud = NULL;	// Python will delete the hud instance
	}

	// Remove ourself from the list
	if( this == m_sHelpers )
	{
		m_sHelpers = m_sHelpers->m_pNext;
		if( m_sHelpers )
			m_sHelpers->m_pPrev = NULL;
	}
	else
	{
		if( m_pNext )
			m_pNext->m_pPrev = m_pPrev;
		if( m_pPrev )
			m_pPrev->m_pNext = m_pNext;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns next object in list
// Output : CHudElementHelper
//-----------------------------------------------------------------------------
CPyHudElementHelper *CPyHudElementHelper::GetNext( void )
{ 
	return m_pNext;
}
