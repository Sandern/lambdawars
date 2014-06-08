//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: Shared code for editor map manager
//
//=============================================================================//

#include "cbase.h"
#include "editorbasemapmgr.h"
#include <filesystem.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef CLIENT_DLL
#define VarArgs UTIL_VarArgs
#endif // CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEditorBaseMapMgr::CEditorBaseMapMgr()
{
	m_szMapError[0] = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
KeyValues *CEditorBaseMapMgr::VmfToKeyValues( const char *pszVmf )
{
	CUtlBuffer vmfBuffer;
	vmfBuffer.SetBufferType( true, true );

	vmfBuffer.SeekPut( CUtlBuffer::SEEK_HEAD, 0 );
	vmfBuffer.PutString( "\"VMFfile\"\r\n{" );

	KeyValues *pszKV = NULL;

	if ( g_pFullFileSystem->ReadFile( pszVmf, NULL, vmfBuffer ) )
	{
		vmfBuffer.SeekPut( CUtlBuffer::SEEK_TAIL, 0 );
		vmfBuffer.PutString( "\r\n}" );
		vmfBuffer.PutChar( '\0' );

		pszKV = new KeyValues("");

		if ( !pszKV->LoadFromBuffer( "", vmfBuffer ) )
		{
			pszKV->deleteThis();
			pszKV = NULL;
		}
	}

	return pszKV;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorBaseMapMgr::KeyValuesToVmf( KeyValues *pKV, CUtlBuffer &vmf )
{
	for ( KeyValues *pKey = pKV->GetFirstTrueSubKey(); pKey; pKey = pKey->GetNextTrueSubKey() )
	{
		pKey->RecursiveSaveToFile( vmf, 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorBaseMapMgr::LogError( const char *pErrorMsg )
{
	Warning("CEditorMapMgr: %s\n", pErrorMsg );
	V_strncpy( m_szMapError, pErrorMsg, sizeof(m_szMapError) );
}