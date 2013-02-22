//====== Copyright © 2013 Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================

#ifndef WARS_PLAT_MISC_H
#define WARS_PLAT_MISC_H
#ifdef _WIN32
#pragma once
#endif

void SrcShellExecute( const char *pCommand );

bool SrcHasProtocolHandler( const char *pProtocol );

#endif // WARS_PLAT_MISC_H