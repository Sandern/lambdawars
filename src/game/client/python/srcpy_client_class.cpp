//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: Represents a Python Network class.
//			A large number of static empty classes are initialized to get around
//			the limitation that you can't dynamically create them.
//			The NetworkedClass can then be created in Python, which will automatically
//			find a matching ClientClass. The server controls which ClientClass the NetworkedClass
//			should pick.
// TODO: Cleanup this file. The pNetworkClassName variables are confusing.
//		 Should be made clear to which one it belongs (the Python or ClientClass one).
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "srcpy_client_class.h"
#include "srcpy.h"
#include "usermessages.h"

#include "basegrenade_shared.h"
#include "Sprite.h"
#include "c_smoke_trail.h"
#include "beam_shared.h"
#include "basecombatweapon_shared.h"
#include "c_playerresource.h"
#include "c_breakableprop.h"
#include "c_basetoggle.h"
#include "c_triggers.h"

#ifdef HL2WARS_DLL
#include "c_hl2wars_player.h"
#include "unit_base_shared.h"
#include "c_wars_weapon.h"
#include "wars_func_unit.h"
#include "wars_mapboundary.h"
#include "wars_network.h"
#endif // HL2WARS_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

PyClientClassBase *g_pPyClientClassHead = NULL;

namespace bp = boost::python;

#if 0
// Recv tables
EXTERN_RECV_TABLE( DT_BaseAnimating );
EXTERN_RECV_TABLE( DT_BaseAnimatingOverlay );
EXTERN_RECV_TABLE( DT_BaseFlex );
EXTERN_RECV_TABLE( DT_BaseCombatCharacter );
EXTERN_RECV_TABLE( DT_BasePlayer );
EXTERN_RECV_TABLE( DT_BaseProjectile );
EXTERN_RECV_TABLE( DT_BaseGrenade );
EXTERN_RECV_TABLE( DT_BaseCombatWeapon );
EXTERN_RECV_TABLE( DT_PlayerResource );
EXTERN_RECV_TABLE( DT_BreakableProp );
EXTERN_RECV_TABLE( DT_BaseToggle );
EXTERN_RECV_TABLE( DT_BaseTrigger );
EXTERN_RECV_TABLE( DT_Sprite );
EXTERN_RECV_TABLE( DT_SmokeTrail );
EXTERN_RECV_TABLE( DT_Beam );

#ifdef HL2WARS_DLL
EXTERN_RECV_TABLE( DT_HL2WarsPlayer );
EXTERN_RECV_TABLE( DT_UnitBase );
EXTERN_RECV_TABLE( DT_WarsWeapon );
EXTERN_RECV_TABLE( DT_FuncUnit );
EXTERN_RECV_TABLE( DT_HL2WarsPlayer );
EXTERN_RECV_TABLE( DT_BaseFuncMapBoundary );
#endif // HL2WARS_DLL

// A lot of factories
#define IMPLEMENT_FALLBACK_FACTORY( clientClassName ) \
	static IClientNetworkable* _PN##clientClassName##_CreateObject( int entnum, int serialNum ) \
	{ \
		clientClassName *pRet = new clientClassName; \
		if ( !pRet ) \
			return 0; \
		pRet->Init( entnum, serialNum ); \
		return pRet; \
	}

#define CALL_FALLBACK_FACTORY( clientClassName, entnum, serialNum ) \
	_PN##clientClassName##_CreateObject(entnum, serialNum);

IMPLEMENT_FALLBACK_FACTORY(C_BaseEntity)
IMPLEMENT_FALLBACK_FACTORY(C_BaseAnimating)
IMPLEMENT_FALLBACK_FACTORY(C_BaseAnimatingOverlay)
IMPLEMENT_FALLBACK_FACTORY(C_BaseFlex)
IMPLEMENT_FALLBACK_FACTORY(C_BaseCombatCharacter)
IMPLEMENT_FALLBACK_FACTORY(C_BasePlayer)
IMPLEMENT_FALLBACK_FACTORY(C_BaseProjectile)
IMPLEMENT_FALLBACK_FACTORY(C_BaseGrenade)
IMPLEMENT_FALLBACK_FACTORY(C_BaseCombatWeapon)
IMPLEMENT_FALLBACK_FACTORY(C_PlayerResource)
IMPLEMENT_FALLBACK_FACTORY(C_BreakableProp)
IMPLEMENT_FALLBACK_FACTORY(C_BaseToggle)
IMPLEMENT_FALLBACK_FACTORY(C_BaseTrigger)
IMPLEMENT_FALLBACK_FACTORY(C_Sprite)
IMPLEMENT_FALLBACK_FACTORY(C_SmokeTrail)
IMPLEMENT_FALLBACK_FACTORY(C_Beam)

#ifdef HL2WARS_DLL
IMPLEMENT_FALLBACK_FACTORY(C_HL2WarsPlayer)
IMPLEMENT_FALLBACK_FACTORY(C_UnitBase)
IMPLEMENT_FALLBACK_FACTORY(C_WarsWeapon)
IMPLEMENT_FALLBACK_FACTORY(C_FuncUnit)
IMPLEMENT_FALLBACK_FACTORY(CBaseFuncMapBoundary)
#endif // HL2WARS_DLL
#endif // 0

IClientNetworkable *ClientClassFactory( boost::python::object cls_type, int entnum, int serialNum )
{
	try	
	{
#if 0
		// Safety check. The base implementations must be match, otherwise it can result in incorrect behavior (crashes)
		int iNetworkType = boost::python::extract<int>(cls_type.attr("GetPyNetworkType")());
		if( iNetworkType != iType )
		{
			char buf[512];
			V_snprintf( buf, sizeof(buf), "Network type does not match client %d != server %d", iNetworkType, iType );
			PyErr_SetString(PyExc_Exception, buf );
			throw boost::python::error_already_set(); 
		}
#endif // 0
		// Spawn and initialize the entity
		boost::python::object inst = cls_type();
		C_BaseEntity *pRet = boost::python::extract<C_BaseEntity *>(inst);
		if( !pRet ) {
			Warning("Invalid client entity\n" );
			return NULL;
		}

		pRet->SetPyInstance( inst );
		pRet->Init( entnum, serialNum );
		return pRet;
	}
	catch(boost::python::error_already_set &)
	{
		Warning("Failed to create python client side entity, falling back to base c++ class\n");
		PyErr_Print();

		// Call the correct fallback factory
		// TODO: is this really needed?
		IClientNetworkable *pResult = NULL;

		return pResult;
	}
}

void InitAllPythonEntities()
{	
	PyClientClassBase *p = g_pPyClientClassHead;
	while( p )
	{
		if( p->m_pNetworkedClass ) {
			p->InitPyClass();
		}
		p = p->m_pPyNext;
	}
}

static CUtlDict< NetworkedClass*, unsigned short > m_NetworkClassDatabase;

PyClientClassBase *FindPyClientClass( const char *pName )
{
	PyClientClassBase *p = g_pPyClientClassHead;
	while( p )
	{
		if ( V_stricmp( p->GetName(), pName ) == 0)
		{
			return p;
		}
		p = p->m_pPyNext;
	}
	return NULL;
}

PyClientClassBase *FindPyClientClassToNetworkClass( const char *pNetworkName )
{
	PyClientClassBase *p = g_pPyClientClassHead;
	while( p )
	{
		if ( V_stricmp( p->m_strPyNetworkedClassName, pNetworkName ) == 0)
		{
			return p;
		}
		p = p->m_pPyNext;
	}
	return NULL;
}

void CheckEntities( PyClientClassBase *pCC, boost::python::object pyClass )
{
	int iHighest = ClientEntityList().GetHighestEntityIndex();
	for ( int i=0; i <= iHighest; i++ )
	{
		C_BaseEntity *pEnt = ClientEntityList().GetBaseEntity( i );
		if ( !pEnt || pEnt->GetClientClass() != pCC || pEnt->GetPyInstance().ptr() == Py_None )
			continue;

		pEnt->GetPyInstance().attr("__setattr__")("__class__", pyClass);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Find a free PyServerClass and claim it
//			Send a message to the clients to claim a client class and set it to
//			the right type.
//-----------------------------------------------------------------------------
NetworkedClass::NetworkedClass( 
							   const char *pNetworkName, boost::python::object cls_type, 
							   const char *pClientModuleName )
{
	m_pClientClass = NULL;
	m_pNetworkName = strdup( pNetworkName );
	m_pyClass = cls_type;
	
	unsigned short lookup = m_NetworkClassDatabase.Find( m_pNetworkName );
	if ( lookup != m_NetworkClassDatabase.InvalidIndex() )
	{
		Warning("NetworkedClass: %s already added, replacing contents...\n", pNetworkName);
		m_pClientClass = m_NetworkClassDatabase[lookup]->m_pClientClass;
		m_NetworkClassDatabase[lookup] = this;
		if( m_pClientClass )
		{
			AttachClientClass( m_pClientClass );
		}
	}
	else
	{
		// New entry, add to database and find if there is an existing ClientClass
		m_NetworkClassDatabase.Insert( m_pNetworkName, this );

		m_pClientClass = FindPyClientClassToNetworkClass( m_pNetworkName );
	}
}

NetworkedClass::~NetworkedClass()
{
	unsigned short lookup;

	// Remove pointer
	lookup = m_NetworkClassDatabase.Find( m_pNetworkName );
	if ( lookup != m_NetworkClassDatabase.InvalidIndex() )
	{
		// Only remove if it's our pointer. Otherwise we are already replaced.
		if( m_NetworkClassDatabase.Element( lookup ) == this )
			m_NetworkClassDatabase.RemoveAt( lookup );
	}
	else
	{
		Warning("NetworkedClass destruction: invalid networkclass %s\n", m_pNetworkName);
	}

	if( m_pClientClass )
	{
		m_pClientClass->m_bFree = true;
	}
	free( (void *)m_pNetworkName );
	m_pNetworkName = NULL;
}

void NetworkedClass::AttachClientClass( PyClientClassBase *pClientClass )
{
	// Free old
	if( m_pClientClass && m_pClientClass->m_pNetworkedClass == this )
	{
		m_pClientClass->m_bFree = true;
		m_pClientClass->m_pNetworkedClass = NULL;
	}

	if( !pClientClass )
	{
		m_pClientClass = NULL;
		return;
	}

	// Attach new 
	m_pClientClass = pClientClass;
	m_pClientClass->m_bFree = false;
	m_pClientClass->m_pNetworkedClass = this;
	try 
	{
		m_pClientClass->SetPyClass(m_pyClass);
		PyObject_SetAttrString(m_pyClass.ptr(), "pyClientClass", bp::object(bp::ptr((ClientClass *)m_pClientClass)).ptr());
	} 
	catch( boost::python::error_already_set & ) 
	{
		PyErr_Print();
	}
}

#define PYNETCLS_BUFSIZE 512

// Message handler for PyNetworkCls
void __MsgFunc_PyNetworkCls( bf_read &msg )
{
	char clientClass[PYNETCLS_BUFSIZE];
	char networkName[PYNETCLS_BUFSIZE];

	msg.ReadString(clientClass, PYNETCLS_BUFSIZE);
	msg.ReadString(networkName, PYNETCLS_BUFSIZE);

	DbgStrPyMsg( "__MsgFunc_PyNetworkCls: Registering Python network class message %s %s\n", clientClass, networkName );

	// Get module path
	const char *pch = V_strrchr( networkName, '.' );
	if( !pch )
	{
		Warning( "Invalid python class name %s\n", networkName );
		return;
	}
	int n = pch - networkName + 1;

	char modulePath[PYNETCLS_BUFSIZE];
	V_strncpy( modulePath, networkName, n );

	// Make sure the client class is imported
	SrcPySystem()->Import( modulePath );

	// Read which client class we are modifying
	PyClientClassBase *p = FindPyClientClass( clientClass );
	if( !p )
	{
		Warning( "__MsgFunc_PyNetworkCls: Invalid networked class %s\n", clientClass );
		return;
	}

	// Read network class name
	V_strncpy( p->m_strPyNetworkedClassName, networkName, sizeof( p->m_strPyNetworkedClassName ) );

	// Attach if a network class exists
	unsigned short lookup = m_NetworkClassDatabase.Find( networkName );
	if ( lookup != m_NetworkClassDatabase.InvalidIndex() )
	{
		m_NetworkClassDatabase.Element(lookup)->AttachClientClass( p );
	}
	else
	{
		Warning( "__MsgFunc_PyNetworkCls: Invalid networked class %s\n", networkName );
	}
}

void WarsNet_ReceiveEntityClasses( CUtlBuffer &data )
{
	if( data.TellMaxPut() <  sizeof(WarsEntityClassesMessage_t) )
	{
		Warning("Received invalid WarsEntityUpdateMessage!\n");
		return;
	}

	//WarsEntityClassesMessage_t *entityClassesMsg = (WarsEntityClassesMessage_t *)data.Base();

	data.SeekGet( CUtlBuffer::SEEK_HEAD, sizeof(WarsEntityClassesMessage_t) );

	int nCount = data.GetInt();

	for( int i = 0; i < nCount; i++ )
	{
		int lenClientClass = data.GetInt();
		char *clientClass = (char *)stackalloc( lenClientClass + 1 );
		data.Get( clientClass, lenClientClass );
		clientClass[lenClientClass] = 0;

		int lenNetworkName = data.GetInt();
		char *networkName = (char *)stackalloc( lenNetworkName + 1 );
		data.Get( networkName, lenNetworkName );
		networkName[lenNetworkName] = 0;
		
		DbgStrPyMsg( "WarsNet_ReceiveEntityClasses: Registering Python network class message %s %s\n", clientClass, networkName );

		// Get module path
		const char *pch = V_strrchr( networkName, '.' );
		if( !pch )
		{
			Warning( "Invalid python class name %s\n", networkName );
			return;
		}
		int n = pch - networkName + 1;

		char modulePath[PYNETCLS_BUFSIZE];
		V_strncpy( modulePath, networkName, n );

		// Make sure the client class is imported
		SrcPySystem()->Import( modulePath );

		// Read which client class we are modifying
		PyClientClassBase *p = FindPyClientClass( clientClass );
		if( !p )
		{
			Warning( "WarsNet_ReceiveEntityClasses: Invalid networked class %s\n", clientClass );
			return;
		}

		// Read network class name
		V_strncpy( p->m_strPyNetworkedClassName, networkName, sizeof( p->m_strPyNetworkedClassName ) );

		// Attach if a network class exists
		unsigned short lookup = m_NetworkClassDatabase.Find( networkName );
		if ( lookup != m_NetworkClassDatabase.InvalidIndex() )
		{
			m_NetworkClassDatabase.Element(lookup)->AttachClientClass( p );
		}
		else
		{
			Warning( "WarsNet_ReceiveEntityClasses: Invalid networked class %s\n", networkName );
		}

		stackfree( clientClass );
		stackfree( networkName );
	}
}

// register message handler once
void HookPyNetworkCls() 
{
	for ( int hh = 0; hh < MAX_SPLITSCREEN_PLAYERS; ++hh )
	{
		ACTIVE_SPLITSCREEN_PLAYER_GUARD( hh );
		usermessages->HookMessage( "PyNetworkCls", __MsgFunc_PyNetworkCls );
	}
}

#if 0
CON_COMMAND_F( rpc, "", FCVAR_HIDDEN )
{
	DbgStrPyMsg( "register_py_class: Registering Python network class message %s %s\n", args[1], args[2] );

	// Get module path
	const char *pch = V_strrchr( args[2], '.' );
	if( !pch )
	{
		Warning("Invalid python class name %s\n", args[2] );
		return;
	}
	int n = pch - args[2] + 1;

	char modulePath[PYNETCLS_BUFSIZE];
	V_strncpy( modulePath, args[2], n );

	SrcPySystem()->Import( modulePath );
	PyClientClassBase *p = FindPyClientClass(args[1]);
	if( !p )
	{
		Warning("register_py_class: Invalid networked class %s\n", args[1]);
		return;
	}

	V_strncpy( p->m_strPyNetworkedClassName, args[2], sizeof( p->m_strPyNetworkedClassName ) );

	// Attach if a network class exists
	unsigned short lookup = m_NetworkClassDatabase.Find( args[2] );
	if ( lookup != m_NetworkClassDatabase.InvalidIndex() )
	{
		m_NetworkClassDatabase.Element(lookup)->AttachClientClass( p );
	}
	else
	{
		Warning( "register_py_class: Invalid networked class %s\n", args[2] );
	}
}
#endif // 0

// Debugging
CON_COMMAND_F( print_py_clientclass_list, "Print client class list", 0 )
{
	if ( !g_pPyClientClassHead )
		return;

	PyClientClassBase *p = g_pPyClientClassHead;
	while( p ) {
		if( p->m_pNetworkedClass )
			Msg("ClientClass: %s linked to %s\n", p->m_pNetworkName, p->m_pNetworkedClass->m_pNetworkName);
		else
			Msg("ClientClass: %s linked to nothing\n", p->m_pNetworkName);
		p = p->m_pPyNext;
	}
}