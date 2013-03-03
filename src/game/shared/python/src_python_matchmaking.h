//====== Copyright © 2013 Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SRC_PYTHON_MATCHMAKING_H
#define SRC_PYTHON_MATCHMAKING_H
#ifdef _WIN32
#pragma once
#endif

class KeyValues;
class ISearchManager;

// IMatchFramework wrappers
void PyMKCreateSession( KeyValues *pSettings );
void PyMKMatchSession( KeyValues *pSettings );
void PyMKCloseSession();

// Python Search Manager
class PySearchManager
{
public:
	PySearchManager();
	~PySearchManager();

	virtual void EnableResultsUpdate( bool bEnable, KeyValues *pSearchParams = NULL );

	virtual int GetNumResults();

	//virtual IMatchSearchResult* GetResultByIndex( int iResultIdx ) = 0;
	//virtual IMatchSearchResult* GetResultByOnlineId( XUID xuidResultOnline ) = 0;

	// Not for python
	void SetSearchManagerInternal( ISearchManager *pSearchManager );

private:
	ISearchManager *m_pSearchManager;
};

// Accessors
class PyMatchSession
{
public:
	static KeyValues * GetSessionSystemData();
	static KeyValues * GetSessionSettings();
	static void UpdateSessionSettings( KeyValues *pSettings );
	static void Command( KeyValues *pCommand );
};

class PyMatchSystem
{
public:
	static bp::object CreateGameSearchManager( KeyValues *pSettings );
};

#endif // SRC_PYTHON_MATCHMAKING_H
