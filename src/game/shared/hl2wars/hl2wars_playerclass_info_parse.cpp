//====== Copyright � 2013 Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "playerclass_info_parse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

FilePlayerClassInfo_t* CreatePlayerClassInfo()
{
	return new FilePlayerClassInfo_t;
}