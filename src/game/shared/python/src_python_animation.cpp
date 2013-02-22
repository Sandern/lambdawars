//====== Copyright © 2013 Sandern Corporation, All rights reserved. ===========//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "src_python_animation.h"
#include "bone_setup.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

float Py_Studio_GetMass( CStudioHdr *pstudiohdr )
{
	if( !pstudiohdr )
		return 0.0;
	return Studio_GetMass(pstudiohdr);
}