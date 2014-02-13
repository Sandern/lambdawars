//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SRCPY_ANIMATION_H
#define SRCPY_ANIMATION_H
#ifdef _WIN32
#pragma once
#endif

#include "studio.h"

#include "srcpy_boostpython.h"

boost::python::object Py_GetSeqdescActivityName( const mstudioseqdesc_t &pstudiohdr );
boost::python::object Py_GetSeqdescLabel( const mstudioseqdesc_t &pstudiohdr );

float Py_Studio_GetMass( CStudioHdr *pstudiohdr );


#endif // SRCPY_ANIMATION_H