//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: CHud handles the message, calculation, and drawing the HUD
//
// $NoKeywords: $
//=============================================================================//
#ifndef SRCPY_HUD_H
#define SRCPY_HUD_H
#ifdef _WIN32
#pragma once
#endif

#include "hud.h"
#include "hud_element_helper.h"

//-----------------------------------------------------------------------------
// Purpose: Used by DECLARE_HUDELEMENT macro to create a linked list of
//  instancing functions
//-----------------------------------------------------------------------------
class CPyHudElementHelper
{
public:
	// Static list of helpers
	static CPyHudElementHelper *m_sHelpers;

public:
	// Construction
	CPyHudElementHelper( boost::python::object hud );
	~CPyHudElementHelper(  );

	// Accessors
	CPyHudElementHelper *GetNext( void );
	inline boost::python::object Get() { return m_pyHud; }

private:
	// Next factory in list
	CPyHudElementHelper	*m_pPrev;
	CPyHudElementHelper	*m_pNext;

	boost::python::object m_pyHud;
	CHudElement *m_pHud;
};

#endif // SRCPY_HUD_H