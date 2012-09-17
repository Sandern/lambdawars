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
#if !defined( SRC_PYTHON_CLIENT_CLASS_H )
#define SRC_PYTHON_CLIENT_CLASS_H
#ifdef _WIN32
#pragma once
#endif

#ifdef DISABLE_PYTHON
	#define IMPLEMENT_PYCLIENTCLASS_SYSTEM( name, network_name )
	#define DECLARE_CREATEPYHANDLE( name )
	#define IMPLEMENT_CREATEPYHANDLE( name )	
	#define DECLARE_PYCLIENTCLASS( name )
	#define IMPLEMENT_PYCLIENTCLASS( name, networkType )
	#define DECLARE_PYCLASS( name )
#else

#include "client_class.h"
#include "src_python_class_shared.h"

extern boost::python::object _entities;

class NetworkedClass;

// Forward declaration
class PyClientClassBase;

// Class head 
extern PyClientClassBase *g_pPyClientClassHead;

namespace DT_BaseEntity
{
	extern RecvTable g_RecvTable;
}

// Use as base for shared things
class PyClientClassBase : public ClientClass
{
public:
	PyClientClassBase( char *pNetworkName ) : ClientClass(pNetworkName, NULL, NULL, NULL), m_pNetworkedClass(NULL)
	{
		m_pRecvTable = &(DT_BaseEntity::g_RecvTable);		// Default
		m_pPyNext				= g_pPyClientClassHead;
		g_pPyClientClassHead	= this;
		m_bFree = true;
	}

	virtual void SetPyClass( boost::python::object cls_type ) {}
	virtual void SetType( int iType ) {}
	virtual void InitPyClass() {}

public:
	PyClientClassBase *m_pPyNext;
	bool m_bFree;
	NetworkedClass *m_pNetworkedClass;
	char m_strPyNetworkedClassName[512];
};

void SetupClientClassRecv( PyClientClassBase *p, int iType );
void PyResetAllNetworkTables();
IClientNetworkable *ClientClassFactory( int iType, boost::python::object cls_type, int entnum, int serialNum);
void InitAllPythonEntities();
void CheckEntities(PyClientClassBase *pCC, boost::python::object pyClass );

// For each available free client class we need a unique class because of the static data
// No way around this since stuff is called from the engine
#define IMPLEMENT_PYCLIENTCLASS_SYSTEM( name, network_name )										\
	class name : public PyClientClassBase															\
	{																								\
	public:																							\
		name() : PyClientClassBase( #network_name )													\
		{																							\
			m_pCreateFn = PyClientClass_CreateObject;												\
		}																							\
		static IClientNetworkable* PyClientClass_CreateObject( int entnum, int serialNum );			\
		virtual void SetPyClass( boost::python::object cls_type );									\
		virtual void SetType( int iType );															\
		virtual void InitPyClass();																	\
	public:																							\
		static boost::python::object m_PyClass;														\
		static int m_iType;																			\
	};																								\
	boost::python::object name##::m_PyClass;														\
	int name##::m_iType;																			\
	IClientNetworkable* name##::PyClientClass_CreateObject( int entnum, int serialNum )				\
	{																								\
		return ClientClassFactory(m_iType, m_PyClass, entnum, serialNum);							\
	}																								\
	void name##::SetPyClass( boost::python::object cls_type )										\
	{																								\
		m_PyClass = cls_type;																		\
		if( g_bDoNotInitPythonClasses == false)														\
			InitPyClass();																			\
		CheckEntities(this, m_PyClass);																\
	}																								\
	void name##::SetType( int iType )																\
	{																								\
		m_iType = iType;																			\
	}																								\
	void name##::InitPyClass()																		\
	{																								\
		bp::object meth = SrcPySystem()->Get("InitEntityClass", m_PyClass, false);					\
		if( meth.ptr() == Py_None )																	\
			return;																					\
		SrcPySystem()->Run<bp::object>( meth, m_PyClass );											\
	}																								\
	name name##_object;																				\

// NetworkedClass is exposed to python and deals with getting a server/client class and cleaning up
class NetworkedClass
{
public:
	NetworkedClass( const char *pNetworkName, boost::python::object cls_type, const char *pClientModuleName=0 );
	~NetworkedClass();

	void AttachClientClass( PyClientClassBase *pClientClass );

public:
	const char *m_pNetworkName;
	PyClientClassBase *m_pClientClass;
	boost::python::object m_pyClass;
};

// Define an handle. Must be done if the entity is exposed to python to convert automatically to the right type
#define DECLARE_CREATEPYHANDLE( name )																\
	virtual boost::python::object CreatePyHandle( void );

#define IMPLEMENT_CREATEPYHANDLE( name )															\
	boost::python::object name::CreatePyHandle( void )												\
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
#define DECLARE_PYCLIENTCLASS( name )																\
	public:																							\
	DECLARE_CREATEPYHANDLE(name);																	\
	static int GetPyNetworkType();

#define IMPLEMENT_PYCLIENTCLASS( name, networkType )												\
	IMPLEMENT_CREATEPYHANDLE(name)																	\
	int name::GetPyNetworkType() { return networkType; }

// Implement a python class. For python/c++ handle conversion
#define DECLARE_PYCLASS( name )																		\
	public:																							\
	boost::python::object CreatePyHandle( void ) const												\
{																								\
	return CreatePyHandleHelper(this, #name "HANDLE");												\
}

#endif // DISABLE_PYTHON

#endif // SRC_PYTHON_CLIENT_CLASS_H