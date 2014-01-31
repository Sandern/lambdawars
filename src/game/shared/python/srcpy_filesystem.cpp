//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "srcpy_filesystem.h"
#include <filesystem.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool PyFS_FileExists( const char *pFileName, const char *pPathID )
{
	return filesystem->FileExists( pFileName, pPathID );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
boost::python::object PyFS_ReadFile( const char *filepath, const char *pathid, bool optimalalloc, int maxbytes, int startingbyte )
{
	void *buffer = NULL;
	int len = filesystem->ReadFileEx( filepath, pathid, &buffer, true, optimalalloc, maxbytes, startingbyte );
	if( !len )
		return boost::python::object();

	boost::python::object content( (const char *)buffer );
	delete buffer; buffer = NULL;
	return content;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
boost::python::object PyFS_FullPathToRelativePath( const char *path, const char *pathid )
{
	char temp[_MAX_PATH];
	if( !filesystem->FullPathToRelativePathEx( path, pathid, temp, _MAX_PATH ) )
		return boost::python::object();
	return boost::python::object(temp);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
boost::python::object PyFS_RelativePathToFullPath( const char *path, const char *pathid )
{
	char temp[_MAX_PATH];
	if( !filesystem->RelativePathToFullPath( path, pathid, temp, _MAX_PATH ) )
		return boost::python::object();
	return boost::python::object(temp);
}