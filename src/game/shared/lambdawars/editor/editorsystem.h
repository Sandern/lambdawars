//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================//

#ifndef EDITORSYSTEM_H
#define EDITORSYSTEM_H
#ifdef _WIN32
#pragma once
#endif

class CEditorSystem : public CBaseGameSystem
{
public:
	// VMF managing
	void LoadCurrentVmf();
#ifndef CLIENT_DLL
	void SaveCurrentVmf();
#endif // CLIENT_DLL
	const char *GetCurrentVmfPath();
	bool IsMapLoaded();

	KeyValues *VmfToKeyValues( const char *pszVmf );
	void KeyValuesToVmf( KeyValues *pKV, CUtlBuffer &vmf );

	void LoadVmf( const char *pszVmf );
	
	

private:
	void ParseVmfFile( KeyValues *pKeyValues );
	
	//void ApplyLightsToCurrentVmfFile();

	void BuildVmfPath( char *pszOut, int maxlen, bool bMakeRelative = true );
	bool BuildCurrentVmfPath( char *pszOut, int maxlen );

#ifndef CLIENT_DLL
	void CollectNewAndUpdatedEntities( CUtlMap< int, CBaseEntity * > &updatedEntities, CUtlVector< CBaseEntity * > &newEntities );
	void ApplyChangesToVmfFile();
#endif // CLIENT_DLL

private:
	// Path to current VMF map file being edited
	char m_szCurrentVmf[MAX_PATH];
	// Data of VMF map file being edited
	KeyValues *m_pKVVmf;
};

CEditorSystem *EditorSystem();

#endif // EDITORSYSTEM_H