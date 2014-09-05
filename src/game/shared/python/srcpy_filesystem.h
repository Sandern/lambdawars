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
long PyFS_GetFileTime( const char *pFileName, const char *pPathID = 0 );
unsigned int PyFS_Size( const char *pFileName, const char *pPathID = 0 );
bool PyFS_IsAbsolutePath( const char *path );

boost::python::object PyFS_ReadFile( const char *filepath, const char *pathid, bool optimalalloc = false, int maxtyes = 0, int startingbyte = 0, bool textmode = false );

boost::python::object PyFS_FullPathToRelativePath( const char *path, const char *pathid = 0, boost::python::object defaultvalue = boost::python::object() );
boost::python::object PyFS_RelativePathToFullPath( const char *path, const char *pathid = 0, boost::python::object defaultvalue = boost::python::object() );

boost::python::list PyFS_ListDir( const char *path, const char *pathid = NULL, const char *wildcard = "*", bool appendslashdir = true );
bool PyFS_IsDirectory( const char *pFileName, const char *pathID = 0 );

#endif // SRCPY_FILESYSTEM_H