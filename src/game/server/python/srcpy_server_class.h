//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//
#if !defined( SRCPY_SERVER_CLASS_H )
#define SRCPY_SERVER_CLASS_H
#ifdef _WIN32
#pragma once
#endif

#ifndef ENABLE_PYTHON
	#define IMPLEMENT_PYSERVERCLASS_SYSTEM( name, network_name )
	#define DECLARE_PYSERVERCLASS( name )
	#define DECLARE_PYCLASS( name )
#else

#include "server_class.h"
#include "dt_send.h"
#include "srcpy_class_shared.h"

class NetworkedClass;
class CBasePlayer;

#define IMPLEMENT_PYSERVERCLASS_SYSTEM( name, network_name, table ) PyServerClass name( #network_name, table );	

class PyServerClass : public ServerClass
{
public:
	PyServerClass( char *pNetworkName, SendTable *pTable );

public:
	PyServerClass *m_pPyNext;
	bool m_bFree;
	NetworkedClass *m_pNetworkedClass;
};

// NetworkedClass is exposed to python and deals with getting a server/client class and cleaning up
class NetworkedClass
{
public:
	NetworkedClass( const char *pNetworkName, boost::python::object cls_type );
	~NetworkedClass();

public:
	boost::python::object m_PyClass;
	const char *m_pNetworkName;

	PyServerClass *m_pServerClass;
};

void FullClientUpdatePyNetworkCls( CBasePlayer *pPlayer );
void FullClientUpdatePyNetworkClsByFilter( IRecipientFilter &filter );
void FullClientUpdatePyNetworkClsByEdict( edict_t *pEdict );
void FullClientUpdatePyNetworkClsByEdict2( edict_t *pEdict );

// Implement a networkable python class. Used to determine the right recv/send tables
#define DECLARE_PYSERVERCLASS( name )													\
	DECLARE_PYCLASS( name )																\
	public:																				\
		static SendTable *GetSendTable() { return m_pClassSendTable; }

#endif // ENABLE_PYTHON

#endif // SRCPY_SERVER_CLASS_H