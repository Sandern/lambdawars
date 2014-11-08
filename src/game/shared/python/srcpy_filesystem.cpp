//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "srcpy_filesystem.h"
#include "srcpy.h"
#include <filesystem.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static bool SrcPyPathIsInGameFolder( const char *pPath )
{
	if( SrcPySystem()->IsPathProtected() )
	{
		// Verify the file is in the gamefolder
		char searchPaths[_MAX_PATH];
		filesystem->GetSearchPath( "MOD", true, searchPaths, sizeof( searchPaths ) );
		V_StripTrailingSlash( searchPaths );

		if( V_IsAbsolutePath(pPath) )
		{
			if( V_strnicmp(pPath, searchPaths, V_strlen(searchPaths)) != 0 ) 
				return false;
		}
		else
		{
			char pFullPath[_MAX_PATH];
			filesystem->RelativePathToFullPath(pPath, "MOD", pFullPath, _MAX_PATH);
			if( V_strnicmp(pFullPath, searchPaths, V_strlen(searchPaths)) != 0 ) 
				return false;
		}
	}
	return true;
}

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
boost::python::object PyFS_ReadFile( const char *filepath, const char *pathid, bool optimalalloc, int maxbytes, int startingbyte, bool textmode )
{
	if( !filepath )
	{
		PyErr_SetString(PyExc_IOError, "No filepath specified" );
		throw boost::python::error_already_set(); 
	}
	if( !SrcPyPathIsInGameFolder( filepath ) )
	{
		PyErr_SetString(PyExc_IOError, "filesystem module only allows paths in the game folder" );
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
	if( textmode )
		content = content.attr("decode")("utf-8"); // TODO: improve. Look at how Python "wt" mode handles text

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
void PyFS_WriteFile( const char *filepath, const char *pathid, const char *content )
{
	if( !filepath )
	{
		PyErr_SetString(PyExc_IOError, "No filepath specified" );
		throw boost::python::error_already_set(); 
	}
	if( !content )
	{
		PyErr_SetString(PyExc_IOError, "Content cannot be empty" );
		throw boost::python::error_already_set(); 
	}

	char convertedPath[MAX_PATH];
	if( V_IsAbsolutePath( filepath ) )
	{
		V_strncpy( convertedPath, filepath, sizeof(convertedPath) );
	}
	else
	{
		char moddir[_MAX_PATH];
		filesystem->RelativePathToFullPath(".", "MOD", moddir, _MAX_PATH);
		V_MakeAbsolutePath( convertedPath, sizeof(convertedPath), filepath, moddir );
	}

	if( !SrcPyPathIsInGameFolder( convertedPath ) )
	{
		PyErr_SetString(PyExc_IOError, "filesystem module only allows paths in the game folder" );
		throw boost::python::error_already_set(); 
	}

	CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );
	buf.Put( content, V_strlen( content ) );
	if( !filesystem->WriteFile( convertedPath, pathid, buf ) )
	{
		PyErr_SetString(PyExc_IOError, "Failed to write file" );
		throw boost::python::error_already_set(); 
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
boost::python::object PyFS_FullPathToRelativePath( const char *path, const char *pathid, boost::python::object defaultvalue )
{
	if( !path )
		return defaultvalue;
	char temp[MAX_PATH];
	if( !filesystem->FullPathToRelativePathEx( path, pathid, temp, sizeof( temp ) ) )
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
	char temp[MAX_PATH];
	if( !filesystem->RelativePathToFullPath( path, pathid, temp, sizeof( temp ) ) )
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