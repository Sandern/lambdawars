//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Client DLL VGUI2 Viewport
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "src_python_sound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static PyEngineSound s_pysoundengine;
PyEngineSound *pysoundengine = &s_pysoundengine;