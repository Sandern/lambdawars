//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "srcpy_server_class.h"
#include "srcpy.h"

#include "wars_network.h"
#include "wars/iwars_extension.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

PyServerClass *g_pPyServerClassHead = NULL;

EXTERN_SEND_TABLE( DT_BaseEntity );
EXTERN_SEND_TABLE( DT_BaseAnimating );
EXTERN_SEND_TABLE( DT_BaseAnimatingOverlay );
EXTERN_SEND_TABLE( DT_BaseFlex );
EXTERN_SEND_TABLE( DT_BaseCombatCharacter );
EXTERN_SEND_TABLE( DT_BasePlayer );
EXTERN_SEND_TABLE( DT_BaseProjectile );
EXTERN_SEND_TABLE( DT_BaseGrenade );
EXTERN_SEND_TABLE( DT_BaseCombatWeapon );
EXTERN_SEND_TABLE( DT_PlayerResource );
EXTERN_SEND_TABLE( DT_BreakableProp );
EXTERN_SEND_TABLE( DT_BaseToggle );
EXTERN_SEND_TABLE( DT_BaseTrigger );

EXTERN_SEND_TABLE( DT_Beam );
EXTERN_SEND_TABLE( DT_Sprite );
EXTERN_SEND_TABLE( DT_SmokeTrail );

#ifdef HL2WARS_DLL
EXTERN_SEND_TABLE( DT_HL2WarsPlayer );
EXTERN_SEND_TABLE( DT_UnitBase );
EXTERN_SEND_TABLE( DT_WarsWeapon );
EXTERN_SEND_TABLE( DT_FuncUnit );
EXTERN_SEND_TABLE( DT_BaseFuncMapBoundary );
#endif // HL2WARS_DLL

namespace bp = boost::python;

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
PyServerClass::PyServerClass( char *pNetworkName, SendTable *pTable ) : ServerClass( pNetworkName, pTable ), m_pNetworkedClass(NULL)
{
	m_pPyNext				= g_pPyServerClassHead;
	g_pPyServerClassHead	= this;
	m_bFree = true;
}

static CUtlDict< const char*, unsigned short > m_ServerClassInfoDatabase;

PyServerClass *FindFreePyServerClass( SendTable *pTable )
{
	PyServerClass *p = g_pPyServerClassHead;
	while( p )
	{
		if( p->m_bFree && p->m_pTable == pTable )
		{
			return p;
		}
		p = p->m_pPyNext;
	}
	return NULL;
}

PyServerClass *FindPyServerClass( const char *pName )
{
	PyServerClass *p = g_pPyServerClassHead;
	while( p )
	{
		if ( _stricmp( p->GetName(), pName ) == 0)
		{
			return p;
		}
		p = p->m_pPyNext;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Find a free PyServerClass and claim it
//			Send a message to the clients to claim a client class and set it to
//			the right type.
//-----------------------------------------------------------------------------
NetworkedClass::NetworkedClass( const char *pNetworkName, boost::python::object cls_type )
{
	m_pNetworkName = strdup( pNetworkName );
	m_pServerClass = NULL;
	PyServerClass *p;

	// See if there is already an entity with this network name
	unsigned short lookup = m_ServerClassInfoDatabase.Find( pNetworkName );
	if ( lookup != m_ServerClassInfoDatabase.InvalidIndex() )
	{
		Warning("NetworkedClass: %s already added. Replacing with new data. Element name: %s\n", pNetworkName, m_ServerClassInfoDatabase.Element(lookup) );
		p = FindPyServerClass( m_ServerClassInfoDatabase.Element(lookup) );
		if( !p )
		{
			Warning("NetworkedClass: ServerClass %s not found\n", m_ServerClassInfoDatabase.Element(lookup) );
			return;
		}
		if( p->m_pNetworkedClass )
			p->m_pNetworkedClass->m_pServerClass = NULL;
	}
	else
	{
		// Find a free server class and add it to the database
		SendTable *pSendTable = boost::python::extract< SendTable *>( cls_type.attr("GetSendTable")() );

		p = FindFreePyServerClass( pSendTable );
		if( !p ) {
			Warning("Couldn't create PyServerClass %s: Out of free PyServerClasses\n", pNetworkName);
			return;
		}

		lookup = m_ServerClassInfoDatabase.Insert(pNetworkName, p->GetName());
	}

	m_pServerClass = p;
	m_PyClass = cls_type;
	p->m_bFree = false;
	p->m_pNetworkedClass = this;

	// Attach to the class
	PyObject_SetAttrString( m_PyClass.ptr(), "pyServerClass", bp::object(bp::ptr((ServerClass *)m_pServerClass)).ptr() );
}

NetworkedClass::~NetworkedClass()
{
	if( m_pServerClass )
	{
		m_pServerClass->m_bFree = true;

		unsigned short lookup = m_ServerClassInfoDatabase.Find( m_pNetworkName );
		if ( lookup != m_ServerClassInfoDatabase.InvalidIndex() )
		{
			m_ServerClassInfoDatabase.Remove( m_pNetworkName );
		}
	}

	if (m_pNetworkName) 
	{
		free( (void *)m_pNetworkName );
	}
	m_pNetworkName = NULL;
}

extern edict_t *g_pForceAttachEdict;

void FullClientUpdatePyNetworkCls( CBasePlayer *pPlayer )
{
	if( !SrcPySystem()->IsPythonRunning() )
		return;
	CSingleUserRecipientFilter filter(pPlayer);
	filter.MakeReliable();
	FullClientUpdatePyNetworkClsByFilter(filter);
}

//-----------------------------------------------------------------------------
// Purpose: Sends Python network class setup through user messages
//-----------------------------------------------------------------------------
void FullClientUpdatePyNetworkClsByFilter( IRecipientFilter &filter )
{
	if( !SrcPySystem()->IsPythonRunning() )
		return;

	// Send messages about each server class
	PyServerClass *p = g_pPyServerClassHead;
	while( p )
	{
		if( p->m_bFree ) {
			p = p->m_pPyNext;
			continue;
		}

		// Send message
		UserMessageBegin(filter, "PyNetworkCls");
		WRITE_STRING(p->m_pNetworkName);
		WRITE_STRING(p->m_pNetworkedClass->m_pNetworkName);
		MessageEnd();

		p = p->m_pPyNext;
	}
}

#if 0
//-----------------------------------------------------------------------------
// Purpose: Sends Python network class setup through commands
//-----------------------------------------------------------------------------
void FullClientUpdatePyNetworkClsByEdict( edict_t *pEdict )
{
	if( !SrcPySystem()->IsPythonRunning() )
	{
		DevMsg("FullClientUpdatePyNetworkClsByEdict: Python is not running\n");
		return;
	}

	Assert(g_SetupNetworkTablesOnHold == false);

	// Send messages about each server class
	PyServerClass *p = g_pPyServerClassHead;
	while( p )
	{
		if( p->m_bFree ) {
			p = p->m_pPyNext;
			continue;
		}

		// Send message
		engine->ClientCommand( pEdict, "rpc %d %s %s\n", p->m_iType,
			p->m_pNetworkName, p->m_pNetworkedClass->m_pNetworkName);
		engine->ServerExecute(); // Send immediately to avoid an overflow when having too many

		p = p->m_pPyNext;
	}
}
#endif //0

//-----------------------------------------------------------------------------
// Purpose: Sends Python network class setup through Steam p2p/loopback
//			TODO: Test with Steam P2P
//-----------------------------------------------------------------------------
void FullClientUpdatePyNetworkClsByEdict( edict_t *pEdict )
{
	if( !SrcPySystem()->IsPythonRunning() )
	{
		DevMsg("FullClientUpdatePyNetworkClsByEdict: Python is not running\n");
		return;
	}

	// Count
	int nCount = 0;
	PyServerClass *p = g_pPyServerClassHead;
	while( p )
	{
		if( p->m_bFree ) {
			p = p->m_pPyNext;
			continue;
		}

		nCount++;
		p = p->m_pPyNext;
	}

	if( !nCount )
		return;

	CUtlBuffer messageData;
	WarsEntityClassesMessage_t baseMessageData;
	baseMessageData.type = k_EMsgClient_PyEntityClasses;

	messageData.Put( &baseMessageData, sizeof( baseMessageData ) );

	messageData.Put( &nCount, sizeof( nCount ) );

	p = g_pPyServerClassHead;
	while( p )
	{
		if( p->m_bFree ) {
			p = p->m_pPyNext;
			continue;
		}

		int lenClientClass = V_strlen( p->m_pNetworkName );
		messageData.Put( &lenClientClass, sizeof( lenClientClass ) );
		messageData.Put( p->m_pNetworkName, lenClientClass );

		int lenNetworkName = V_strlen( p->m_pNetworkedClass->m_pNetworkName );
		messageData.Put( &lenNetworkName, sizeof( lenNetworkName ) );
		messageData.Put( p->m_pNetworkedClass->m_pNetworkName, lenNetworkName );

		p = p->m_pPyNext;
	}

	// Always loopback for now
	WarsMessageData_t *pMessageData = warsextension->InsertClientMessage();
	pMessageData->buf.Put( messageData.Base(), messageData.TellMaxPut() );
}

// Debugging
CON_COMMAND_F( print_py_serverclass_list, "Print server class list", 0)
{
	if ( !g_pPyServerClassHead )
		return;

	PyServerClass *p = g_pPyServerClassHead;
	while( p ) {
		if( p->m_pNetworkedClass )
			Msg("ServerClass: %s linked to %s\n", p->m_pNetworkName, p->m_pNetworkedClass->m_pNetworkName);
		else
			Msg("ServerClass: %s linked to nothing\n", p->m_pNetworkName);
		p = p->m_pPyNext;
	}
}
