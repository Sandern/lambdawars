//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SRCPY_FILESYSTEM_H
#define SRCPY_FILESYSTEM_H
#ifdef _WIN32
#pragma once
#endif

#include "srcpy_boostpython.h"

bool PyFS_FileExists( const char *pFileName, const char *pPathID = 0 );

boost::python::object PyFS_ReadFile( const char *filepath, const char *pathid, bool optimalalloc = false, int maxtyes = 0, int startingbyte = 0 );

boost::python::object PyFS_FullPathToRelativePath( const char *path, const char *pathid = 0 );
boost::python::object PyFS_RelativePathToFullPath( const char *path, const char *pathid = 0 );

#endif // SRCPY_FILESYSTEM_H