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
	if( !pFileName )
		return false;
	return filesystem->FileExists( V_strlen(pFileName) == 0 ? "." : pFileName, pPathID );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
long PyFS_GetFileTime( const char *pFileName, const char *pPathID )
{
	if( !pFileName )
		return -1;
	return filesystem->GetFileTime( V_strlen(pFileName) == 0 ? "." : pFileName, pPathID );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
unsigned int PyFS_Size( const char *pFileName, const char *pPathID )
{
	if( !pFileName )
		return 0;
	return filesystem->Size( pFileName, pPathID );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool PyFS_IsAbsolutePath( const char *path )
{
	if( !path )
		return false;
	return V_IsAbsolutePath( path );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
boost::python::object PyFS_ReadFile( const char *filepath, const char *pathid, bool optimalalloc, int maxbytes, int startingbyte )
{
	if( !filepath )
	{
		PyErr_SetString(PyExc_IOError, "No filepath specified" );
		throw boost::python::error_already_set(); 
	}
	if( !filesystem->FileExists( filepath, pathid ) )
	{
		PyErr_SetString(PyExc_IOError, "File does not exists" );
		throw boost::python::error_already_set(); 
	}

	int iExpectedSize = filesystem->Size( filepath, pathid );
	void *buffer = NULL;
	int len = filesystem->ReadFileEx( filepath, pathid, &buffer, true, optimalalloc, maxbytes, startingbyte );
	if( len != iExpectedSize )
	{
		PyErr_SetString(PyExc_IOError, "Failed to read file" );
		throw boost::python::error_already_set(); 
	}

	boost::python::object content( boost::python::handle<>( PyBytes_FromStringAndSize( (const char *)buffer, (Py_ssize_t)len ) ) );

	if( optimalalloc )
		filesystem->FreeOptimalReadBuffer( buffer );
	else
		free( buffer );
	buffer = NULL;
	return content;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
boost::python::object PyFS_FullPathToRelativePath( const char *path, const char *pathid, boost::python::object defaultvalue )
{
	if( !path )
		return defaultvalue;
	char temp[_MAX_PATH];
	if( !filesystem->FullPathToRelativePathEx( path, pathid, temp, _MAX_PATH ) )
		return defaultvalue;
	return boost::python::object(temp);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
boost::python::object PyFS_RelativePathToFullPath( const char *path, const char *pathid, boost::python::object defaultvalue )
{
	if( !path )
		return defaultvalue;
	char temp[_MAX_PATH];
	if( !filesystem->RelativePathToFullPath( path, pathid, temp, _MAX_PATH ) )
		return defaultvalue;
	return boost::python::object(temp);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
boost::python::list PyFS_ListDir( const char *pPath, const char *pPathID, const char *pWildCard, bool appendslashdir )
{
	if( !pPath || !pWildCard )
		return boost::python::list();

	const char *pFileName;
	char wildcard[MAX_PATH];
	FileFindHandle_t fh;
	boost::python::list result;

	// Construct the wildcard
	V_strncpy( wildcard, pPath, sizeof( wildcard ) );
	if( V_strlen( wildcard ) == 0 )
		V_strncpy( wildcard, "./", sizeof( wildcard ) );
	if( appendslashdir && filesystem->IsDirectory( pPath ) )
		V_AppendSlash( wildcard, sizeof( wildcard ) );
	V_strcat( wildcard, pWildCard, sizeof( wildcard ) );

	pFileName = filesystem->FindFirstEx( wildcard, pPathID, &fh );
	while( pFileName )
	{
		result.append( boost::python::object( pFileName ) );
		pFileName = filesystem->FindNext( fh );
	}
	filesystem->FindClose( fh );
	return result;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool PyFS_IsDirectory( const char *pFileName, const char *pathID )
{
	if( !pFileName )
		return false;
	return filesystem->IsDirectory( pFileName, pathID );
}