//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "gamerules_register.h"

// =======================================
// PySource Additions
// =======================================
#ifdef ENABLE_PYTHON
#include "srcpy_gamerules.h"
#endif // ENABLE_PYTHON
// =======================================
// END PySource Additions
// =======================================

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// ------------------------------------------------------------------------------------------ //
// CGameRulesRegister implementation.
// ------------------------------------------------------------------------------------------ //

CGameRulesRegister* CGameRulesRegister::s_pHead = NULL;


CGameRulesRegister::CGameRulesRegister( const char *pClassName, CreateGameRulesFn fn )
{
	m_pClassName = pClassName;
	m_pFn = fn;
	
	m_pNext = s_pHead;	// Add us to the global list.
	s_pHead = this;
}

void CGameRulesRegister::CreateGameRules()
{
	m_pFn();
// =======================================
// PySource Additions
// =======================================
#ifdef ENABLE_PYTHON
	// Add Gamerules as game system here.
	// Basically we don't do this automatically to prevent 
	// non cleaned up gamerules from being still registered
	IGameSystem::Add( g_pGameRules );
#endif // ENABLE_PYTHON
// =======================================
// END PySource Additions
// =======================================
}

CGameRulesRegister* CGameRulesRegister::FindByName( const char *pName )
{
	for ( CGameRulesRegister *pCur=s_pHead; pCur; pCur=pCur->m_pNext )
	{
		if ( Q_stricmp( pName, pCur->m_pClassName ) == 0 )
			return pCur;
	}
	return NULL;
}


// ------------------------------------------------------------------------------------------ //
// Functions to dispatch the messages to create the game rules object on the client.
// ------------------------------------------------------------------------------------------ //

#define GAMERULES_STRINGTABLE_NAME "GameRulesCreation"


#ifdef CLIENT_DLL

	#include "networkstringtable_clientdll.h"

	INetworkStringTable *g_StringTableGameRules = NULL;

	void OnGameRulesCreationStringChanged( 
		void *object, 
		INetworkStringTable *stringTable, 
		int stringNumber, 
		const char *newString, 
		void const *newData )
	{
		// The server has created a new CGameRules object.
// =======================================
// PySource Additions
// =======================================
#ifdef ENABLE_PYTHON
		if( PyGameRules().ptr() != Py_None )
			ClearPyGameRules();		// Python managed	
		else 
#endif // ENABLE_PYTHON
// =======================================
// END PySource Additions
// =======================================
		{
			delete g_pGameRules;
			g_pGameRules = NULL;
		}

		const char *pClassName = (const char*)newData;
		CGameRulesRegister *pReg = CGameRulesRegister::FindByName( pClassName );
		if ( !pReg )
			Error( "OnGameRulesCreationStringChanged: missing gamerules class '%s' on the client", pClassName );

		// Create the new game rules object.
		pReg->CreateGameRules();

		if ( !g_pGameRules )
		{
			Error( "OnGameRulesCreationStringChanged: game rules entity (%s) not created", pClassName );
		}
	}

	// On the client, we respond to string table changes on the server.
	void InstallStringTableCallback_GameRules( const char *tableName )
	{
		if ( 0 == Q_strcasecmp( tableName, GAMERULES_STRINGTABLE_NAME ) )
		{
			g_StringTableGameRules = networkstringtable->FindTable( tableName );
			if ( g_StringTableGameRules )
				g_StringTableGameRules->SetStringChangedCallback( NULL, OnGameRulesCreationStringChanged );
		}
	}

#else

	#include "networkstringtable_gamedll.h"

	INetworkStringTable *g_StringTableGameRules = NULL;

	void CreateNetworkStringTables_GameRules()
	{
		// Create the string tables
		g_StringTableGameRules = networkstringtable->CreateStringTable( GAMERULES_STRINGTABLE_NAME, 1 );


	}

	void CreateGameRulesObject( const char *pClassName )
	{
		// Delete the old game rules object.
// =======================================
// PySource Additions
// =======================================
#ifdef ENABLE_PYTHON
		if( PyGameRules().ptr() != Py_None )
			ClearPyGameRules();	
		else
#endif // ENABLE_PYTHON
// =======================================
// END PySource Additions
// =======================================
			delete g_pGameRules;
		g_pGameRules = NULL;

		// Create a new game rules object.
		CGameRulesRegister *pReg = CGameRulesRegister::FindByName( pClassName );
		if ( !pReg )
			Error( "InitGameRules: missing gamerules class '%s' on the server", pClassName );
	
		pReg->CreateGameRules();
		if ( !g_pGameRules )
		{
			Error( "InitGameRules: game rules entity (%s) not created", pClassName );
		}

		// Make sure the client gets notification to make a new game rules object.
		Assert( g_StringTableGameRules );
		g_StringTableGameRules->AddString( true, "classname", strlen( pClassName ) + 1, pClassName );

		if ( g_pGameRules )
		{
			g_pGameRules->CreateCustomNetworkStringTables();
		}
	}			

#endif
	
