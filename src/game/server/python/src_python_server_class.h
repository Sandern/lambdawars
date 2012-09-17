//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
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
#if !defined( SRC_PYTHON_SERVER_CLASS_H )
#define SRC_PYTHON_SERVER_CLASS_H
#ifdef _WIN32
#pragma once
#endif

#ifdef DISABLE_PYTHON
	#define IMPLEMENT_PYCLIENTCLASS_SYSTEM( name, network_name )
	#define DECLARE_CREATEPYHANDLE( name )
	#define IMPLEMENT_CREATEPYHANDLE( name )	
	#define DECLARE_PYSERVERCLASS( name )
	#define IMPLEMENT_PYSERVERCLASS( name, networkType )
	#define DECLARE_PYCLASS( name )
#else

#include "server_class.h"
#include "dt_send.h"
#include "src_python_class_shared.h"

extern boost::python::object _entities;

class NetworkedClass;
class CBasePlayer;

#define IMPLEMENT_PYSERVERCLASS_SYSTEM( name, network_name ) PyServerClass name(#network_name);	

class PyServerClass : public ServerClass
{
public:
	PyServerClass(char *pNetworkName);

	void SetupServerClass( int iType );

public:
	SendProp *m_pSendProps;
	PyServerClass *m_pPyNext;
	bool m_bFree;
	int m_iType;
	NetworkedClass *m_pNetworkedClass;
};

// NetworkedClass is exposed to python and deals with getting a server/client class and cleaning up
class NetworkedClass
{
public:
	NetworkedClass( const char *pNetworkName, boost::python::object cls_type, const char *pClientModuleName );
	~NetworkedClass();

	void SetupServerClass();

public:
	boost::python::object m_PyClass;
	const char *m_pNetworkName;
	const char *m_pClientModuleName;

	PyServerClass *m_pServerClass;
};

void FullClientUpdatePyNetworkCls( CBasePlayer *pPlayer );
void FullClientUpdatePyNetworkClsByFilter( IRecipientFilter &filter );
void FullClientUpdatePyNetworkClsByEdict( edict_t *pEdict );

typedef struct EntityInfoOnHold {
	CBaseEntity *ent;
	edict_t *edict;
} EntityInfoOnHold;

void SetupNetworkTables();
void SetupNetworkTablesOnHold();
void AddSetupNetworkTablesOnHoldEnt( EntityInfoOnHold info );
bool SetupNetworkTablesRelease();
void PyResetAllNetworkTables();

// Define an handle. Must be done if the entity is exposed to python to convert automatically to the right type
#define DECLARE_CREATEPYHANDLE( name )																\
	virtual boost::python::object CreatePyHandle( void ) const;

#define IMPLEMENT_CREATEPYHANDLE( name )															\
	boost::python::object name::CreatePyHandle( void ) const										\
	{																								\
		boost::python::object clshandle;															\
		if( GetPyInstance().ptr() != Py_None )														\
		{																							\
			try {																					\
				clshandle = _entities.attr("PyHandle");												\
			} catch(boost::python::error_already_set &) {											\
				Warning("Failed to create a PyHandle\n");											\
				PyErr_Print();																		\
				PyErr_Clear();																		\
				return boost::python::object();														\
			}																						\
			return clshandle(GetPyInstance());														\
		}																							\
		try {																						\
			clshandle = _entities.attr(#name "HANDLE");												\
		} catch(boost::python::error_already_set &) {												\
			Warning("Failed to create handle %s\n", #name "HANDLE");								\
			PyErr_Print();																			\
			PyErr_Clear();																			\
			return boost::python::object();															\
		}																							\
		return clshandle(boost::python::ptr(this));													\
	}

// Implement a networkable python class. Used to determine the right recv/send tables
#define DECLARE_PYSERVERCLASS( name )																\
	public:																							\
	DECLARE_CREATEPYHANDLE(name);																	\
	static int GetPyNetworkType();

#define IMPLEMENT_PYSERVERCLASS( name, networkType )												\
	IMPLEMENT_CREATEPYHANDLE(name);																	\
	int name::GetPyNetworkType() { return networkType; }

// Implement a python class. For python/c++ handle conversion
#define DECLARE_PYCLASS( name )																		\
	public:																							\
	boost::python::object CreatePyHandle( void ) const												\
	{																								\
		return CreatePyHandleHelper(this, #name "HANDLE");												\
	}

#endif // DISABLE_PYTHON

#endif // SRC_PYTHON_SERVER_CLASS_H