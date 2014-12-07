//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================//

#ifndef SRCPY_NETWORKVAR_H
#define SRCPY_NETWORKVAR_H

#ifdef _WIN32
#pragma once
#endif

#include "srcpy_boostpython.h"

extern ConVar g_debug_pynetworkvar;

#ifndef CLIENT_DLL

// Python Send proxies
class CPythonSendProxyBase
{
public:
	virtual bool ShouldSend( CBaseEntity *pEnt, int iClient ) { return true; }
};

class CPythonSendProxyOwnerOnly : public CPythonSendProxyBase
{
public:
	virtual bool ShouldSend( CBaseEntity *pEnt, int iClient );
};

class CPythonSendProxyAlliesOnly : public CPythonSendProxyBase
{
public:
	virtual bool ShouldSend( CBaseEntity *pEnt, int iClient );
};

// Python network classes
#define PYNETVAR_MAX_NAME 260
class CPythonNetworkVarBase
{
public:
	CPythonNetworkVarBase( boost::python::object ent, const char *name, bool changedcallback=false, boost::python::object sendproxy=boost::python::object() );
	~CPythonNetworkVarBase();

	void Remove( CBaseEntity *pEnt );

	void NetworkStateChanged( void );
	virtual void NetworkVarsUpdateClient( CBaseEntity *pEnt, int iClient ) {}

	// For optimization purposes. Returns if the data is in the unmodified state at construction.
	bool IsInInitialState() { return m_bInitialState; }

public:
	// This bit vector contains the players who don't have the most up to date data
	CBitVec<ABSOLUTE_PLAYER_LIMIT> m_PlayerUpdateBits;

protected:
	char m_Name[PYNETVAR_MAX_NAME];
	bool m_bChangedCallback;
	bool m_bInitialState;

	boost::python::object m_wrefEnt;

	CPythonSendProxyBase *m_pPySendProxy;
	boost::python::object m_pySendProxyRef;
};

class CPythonNetworkVar : public CPythonNetworkVarBase
{
public:
	CPythonNetworkVar( boost::python::object self, const char *name, boost::python::object data = boost::python::object(), 
		bool initstatechanged=false, bool changedcallback=false, boost::python::object sendproxy=boost::python::object() );

	void NetworkVarsUpdateClient( CBaseEntity *pEnt, int iClient );

	void Set( boost::python::object data );
	boost::python::object Get( void );
	boost::python::object Str();

private:
	boost::python::object m_dataInternal;
};

class CPythonNetworkArray : public CPythonNetworkVarBase
{
public:
	CPythonNetworkArray( boost::python::object self, const char *name, boost::python::list data = boost::python::list(), 
		bool initstatechanged=false, bool changedcallback=false, boost::python::object sendproxy=boost::python::object() );

	void NetworkVarsUpdateClient( CBaseEntity *pEnt, int iClient );

	void SetItem( boost::python::object key, boost::python::object data );
	boost::python::object GetItem( boost::python::object key );
	void DelItem( boost::python::object key );

	void Set( boost::python::list data );
	boost::python::object Str();

private:
	boost::python::list m_dataInternal;
};

class CPythonNetworkDict : public CPythonNetworkVarBase
{
public:
	CPythonNetworkDict( boost::python::object self, const char *name, boost::python::dict data = boost::python::dict(), 
		bool initstatechanged=false, bool changedcallback=false, boost::python::object sendproxy=boost::python::object() );

	void NetworkVarsUpdateClient( CBaseEntity *pEnt, int iClient );

	void SetItem( boost::python::object key, boost::python::object data );
	boost::python::object GetItem(  boost::python::object key );

	void Set( boost::python::dict data );
	boost::python::object Str();

private:
	boost::python::dict m_dataInternal;

#if 0
	std::set<boost::python::object> m_changedKeys;
#endif // 0
};

// Reset all network variables transmit bits for the specified client (0 based)
void PyNetworkVarsResetClientTransmitBits( int iClient );
// Updates network variables for specified client index (0 based)
void PyNetworkVarsUpdateClient( CBaseEntity *pEnt, int iClient );

#else
	void HookPyNetworkVar();
#endif // CLIENT_DLL

#endif // SRCPY_NETWORKVAR_H