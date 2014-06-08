//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================//

#ifndef EDITORBASEMAPMGR_H
#define EDITORBASEMAPMGR_H
#ifdef _WIN32
#pragma once
#endif

class CEditorBaseMapMgr
{
public:
	CEditorBaseMapMgr();

	const char *GetLastMapError();

	// Map mgr
	virtual bool IsMapLoaded() = 0;

	// Common
	KeyValues *VmfToKeyValues( const char *pszVmf );
	void KeyValuesToVmf( KeyValues *pKV, CUtlBuffer &vmf );

protected:
	void LogError( const char *pErrorMsg );

private:
	// Last error message during a map operation
	char m_szMapError[1024];
};

inline const char *CEditorBaseMapMgr::GetLastMapError()
{
	return m_szMapError;
}

#endif // EDITORBASEMAPMGR_H