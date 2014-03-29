//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//===========================================================================//

#include "cbase.h"
#include "srcpy_networkvar.h"
#include "srcpy.h"
#include "srcpy_usermessage.h"
#include "unit_base_shared.h"

#ifndef CLIENT_DLL
#include "enginecallback.h"
#else
#include "usermessages.h"
#endif // CLIENT_DLL

ConVar g_debug_pynetworkvar("g_debug_pynetworkvar", "0", FCVAR_CHEAT|FCVAR_REPLICATED);

namespace bp = boost::python;

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Send proxies
//-----------------------------------------------------------------------------
bool CPythonSendProxyOwnerOnly::ShouldSend( CBaseEntity *pEnt, int iClient )
{
	CBasePlayer *pPlayer = UTIL_PlayerByIndex( iClient + 1 );
	if( !pPlayer )
		return false;

	return pPlayer->GetOwnerNumber() == pEnt->GetOwnerNumber();
}

bool CPythonSendProxyAlliesOnly::ShouldSend( CBaseEntity *pEnt, int iClient )
{
	CBasePlayer *pPlayer = UTIL_PlayerByIndex( iClient + 1 );
	if( !pPlayer )
		return false;

	return GetPlayerRelationShip( pEnt->GetOwnerNumber(), pPlayer->GetOwnerNumber() ) != D_HATE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPythonNetworkVarBase::CPythonNetworkVarBase( bp::object ent, const char *name, bool changedcallback, bp::object sendproxy )
	: m_bChangedCallback(changedcallback), m_bInitialState(true)
{
	Q_snprintf(m_Name, PYNETVAR_MAX_NAME, name);
	CBaseEntity *pEnt = NULL;
	m_pPySendProxy = NULL;
	try 
	{
		pEnt = bp::extract<CBaseEntity *>(ent);
		if( sendproxy.ptr() != Py_None )
			m_pPySendProxy = bp::extract<CPythonSendProxyBase *>(sendproxy);
	} 
	catch(boost::python::error_already_set &) 
	{
		PyErr_Print();
		PyErr_Clear();
		return;
	}

	m_pySendProxyRef = sendproxy;

	m_wrefEnt = SrcPySystem()->CreateWeakRef(ent);
	if( pEnt ) 
	{
		pEnt->m_utlPyNetworkVars.AddToTail(this);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPythonNetworkVarBase::~CPythonNetworkVarBase()
{
	if( m_wrefEnt.ptr() == Py_None )
		return;

	CBaseEntity *pEnt = NULL;
	try 
	{
		pEnt = bp::extract<CBaseEntity *>(m_wrefEnt());
	} 
	catch(boost::python::error_already_set &) 
	{
		PyErr_Print();
		PyErr_Clear();
		return;
	}

	Remove( pEnt );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPythonNetworkVarBase::Remove( CBaseEntity *pEnt )
{
	if( !pEnt )
		return;

	pEnt->m_utlPyNetworkVars.FindAndRemove( this );
	m_wrefEnt = bp::object();
	m_pySendProxyRef = bp::object();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPythonNetworkVarBase::NetworkStateChanged( void )
{
	m_PlayerUpdateBits.SetAll();
	m_bInitialState = false;
}

//---------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPythonNetworkVar::CPythonNetworkVar( bp::object ent, const char *name, bp::object data, 
									 bool bInitStateChanged, bool changedcallback, bp::object sendproxy )
			: CPythonNetworkVarBase( ent, name, changedcallback, sendproxy )
{
	m_dataInternal = data;
	if( bInitStateChanged )
		NetworkStateChanged();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPythonNetworkVar::Set( bp::object data )
{
	if( data.ptr() == m_dataInternal.ptr() )
		return;
	m_dataInternal = data;
	NetworkStateChanged();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bp::object CPythonNetworkVar::Get( void )
{
	return m_dataInternal;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPythonNetworkVar::NetworkVarsUpdateClient( CBaseEntity *pEnt, int iClient )
{
	if( m_pPySendProxy && !m_pPySendProxy->ShouldSend( pEnt, iClient ) )
		return;

	m_PlayerUpdateBits.Clear(iClient);

	pywrite write;
	try 
	{
		PyFillWriteElement( write, Get() );
	} 
	catch(boost::python::error_already_set &) 
	{
		Warning("Failed to parse data for network variable %s:\n", m_Name );
		PyErr_Print();
		PyErr_Clear();
		Set( bp::object(0) );
		return;
	}

	CRecipientFilter filter;
	filter.MakeReliable();
	filter.AddRecipient( iClient + 1 );
	if( m_bChangedCallback )
		UserMessageBegin( filter, "PyNetworkVarCC" );
	else
		UserMessageBegin( filter, "PyNetworkVar");
	WRITE_EHANDLE(pEnt);
	WRITE_STRING(m_Name);
	PyWriteElement(write);
	MessageEnd();

	if( g_debug_pynetworkvar.GetBool() )
	{
		DevMsg("#%d:%s - %f - PyNetworkVar: %s, Value -> ", pEnt->entindex(), pEnt->GetClassname(), gpGlobals->curtime, m_Name);
		PyPrintElement(write);
	}
}

//---------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPythonNetworkArray::CPythonNetworkArray( bp::object ent, const char *name, bp::list data, 
										 bool bInitStateChanged, bool changedcallback, bp::object sendproxy )
: CPythonNetworkVarBase( ent, name, changedcallback, sendproxy )
{
	if(data.ptr() != Py_None)
	{
		m_dataInternal = data;
		if( bInitStateChanged )
			NetworkStateChanged();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPythonNetworkArray::SetItem( bp::object key, bp::object data )
{
	if( data == boost::python::object(m_dataInternal[key]) )
		return;
	m_dataInternal[key] = data;
	NetworkStateChanged();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bp::object CPythonNetworkArray::GetItem( bp::object key )
{
	return m_dataInternal[key];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPythonNetworkArray::DelItem( bp::object key )
{
	m_dataInternal.attr("__delitem__")(key);
	NetworkStateChanged();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPythonNetworkArray::Set( bp::list data )
{
	m_dataInternal = data;
	NetworkStateChanged();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPythonNetworkArray::NetworkVarsUpdateClient( CBaseEntity *pEnt, int iClient )
{
	if( m_pPySendProxy && !m_pPySendProxy->ShouldSend( pEnt, iClient ) )
		return;

	m_PlayerUpdateBits.Clear(iClient);

	// Parse list
	int length = 0;
	CUtlVector<pywrite> writelist;
	try 
	{
		length = boost::python::len(m_dataInternal);
		for( int i = 0; i < length; i++ )
		{
			pywrite write;
			PyFillWriteElement( write, boost::python::object(m_dataInternal[i]) );
			writelist.AddToTail(write);
		}
	} 
	catch(boost::python::error_already_set &) 
	{
		PyErr_Print();
		PyErr_Clear();
		return;
	}

	CRecipientFilter filter;
	filter.MakeReliable();
	filter.AddRecipient( iClient + 1 );
	if( m_bChangedCallback )
		UserMessageBegin( filter, "PyNetworkArrayFullCC" );
	else
		UserMessageBegin( filter, "PyNetworkArrayFull");
	WRITE_EHANDLE(pEnt);
	WRITE_STRING(m_Name);
	WRITE_BYTE(length);
	for( int i = 0; i < writelist.Count(); i++ )
	{
		PyWriteElement(writelist.Element(i));
	}
	MessageEnd();

	if( g_debug_pynetworkvar.GetBool() )
	{
		DevMsg("#%d:%s - %f - PyNetworkArray: %s\n", pEnt->entindex(), pEnt->GetClassname(), gpGlobals->curtime, m_Name);
		for( int i = 0; i < writelist.Count(); i++ )
		{
			DevMsg("\t");
			PyPrintElement(writelist.Element(i));
		}
	}
}

//---------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPythonNetworkDict::CPythonNetworkDict( bp::object ent, const char *name, bp::dict data, 
									   bool bInitStateChanged, bool changedcallback, bp::object sendproxy )
: CPythonNetworkVarBase( ent, name, changedcallback, sendproxy )
{
	if(data.ptr() != Py_None)
	{
		m_dataInternal = data;
		if( bInitStateChanged )
			NetworkStateChanged();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPythonNetworkDict::SetItem( bp::object key, bp::object data )
{
	try {
		if( data == bp::object(m_dataInternal[key]) )
			return;
	} catch( bp::error_already_set & ) {
		PyErr_Clear();
	}

#if 0
	m_changedKeys.insert(key);
#endif // 0
	m_dataInternal[key] = data;
	NetworkStateChanged();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bp::object CPythonNetworkDict::GetItem( bp::object key )
{
	return m_dataInternal[key];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPythonNetworkDict::Set( bp::dict data )
{
	m_dataInternal = data;
	NetworkStateChanged();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPythonNetworkDict::NetworkVarsUpdateClient( CBaseEntity *pEnt, int iClient )
{
	if( m_pPySendProxy && !m_pPySendProxy->ShouldSend( pEnt, iClient ) )
		return;

	m_PlayerUpdateBits.Clear(iClient);

	// Create write list
	// TODO: Only write changed keys if possible
	unsigned long length = 0;
	CUtlVector<pywrite> writelist;
	try 
	{
#if 0
		if( m_changedKeys.size() > 0 )
		{
			// Send changed keys
			m_changedKeys.clear();
		}
		else
#endif // 0
		{
			bp::dict dictdata( bp::detail::borrowed_reference(m_dataInternal.ptr()) );

			bp::object items = dictdata.attr("items")();
			bp::object iterator = items.attr("__iter__")();
			length = bp::len(items); 
			for( unsigned long u = 0; u < length; u++ )
			{
				bp::object item = iterator.attr( PY_NEXT_METHODNAME )();

				pywrite write;
				PyFillWriteElement( write, item[0] );
				writelist.AddToTail(write);

				pywrite write2;
				PyFillWriteElement( write2, item[1] );
				writelist.AddToTail(write2);
			}
		}
	} 
	catch( boost::python::error_already_set & ) 
	{
		PyErr_Print();
		PyErr_Clear();
		return;
	}

	// Send message
	CRecipientFilter filter;
	filter.MakeReliable();
	filter.AddRecipient( iClient + 1 );
	UserMessageBegin( filter, m_bChangedCallback ? "PyNetworkDictFullCC" : "PyNetworkDictFull" );
	WRITE_EHANDLE(pEnt);
	WRITE_STRING(m_Name);
	WRITE_BYTE((int)length);
	for( int i = 0; i < writelist.Count(); i++ )
	{
		PyWriteElement(writelist.Element(i));
	}
	MessageEnd();

	if( g_debug_pynetworkvar.GetBool() )
	{
		DevMsg("#%d:%s - %f - PyNetworkDict: %s (length: %d)\n", pEnt->entindex(), pEnt->GetClassname(), gpGlobals->curtime, m_Name, (int)length);
		for( int i = 0; i < writelist.Count(); i++ )
		{
			DevMsg("\t");
			PyPrintElement(writelist.Element(i));
		}
	}
}

//---------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PyNetworkVarsResetClientTransmitBits( int iClient )
{
	// All Python network vars also need to be resend...
	CBaseEntity *pEnt = gEntList.FirstEnt();
	while( pEnt )
	{
		for( int i = 0; i < pEnt->m_utlPyNetworkVars.Count(); i++ )
		{
			if( pEnt->m_utlPyNetworkVars[i]->IsInInitialState() )
				continue;

			pEnt->m_utlPyNetworkVars[i]->m_PlayerUpdateBits.Set( iClient );
		}

		pEnt = gEntList.NextEnt( pEnt );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PyNetworkVarsUpdateClient( CBaseEntity *pEnt, int iClient )
{
	if( !pEnt->m_utlPyNetworkVars.Count() )
		return;

	if( pEnt->m_PyNetworkVarsPlayerTransmitBits.Get( iClient ) == false )
		return;

	for( int i = 0; i < pEnt->m_utlPyNetworkVars.Count(); i++ )
	{
		if( pEnt->m_utlPyNetworkVars.Element( i )->m_PlayerUpdateBits.Get( iClient ) == false )
			continue;

		pEnt->m_utlPyNetworkVars.Element( i )->NetworkVarsUpdateClient( pEnt, iClient );
	}
}

#else 


//-----------------------------------------------------------------------------
// Purpose: Messaging
//-----------------------------------------------------------------------------
EHANDLE ReadEHANLE( bf_read &msg )
{
	long iEncodedEHandle;
	int iSerialNum, iEntryIndex;

	iEncodedEHandle = msg.ReadLong();
	iSerialNum = (iEncodedEHandle >> MAX_EDICT_BITS);
	iEntryIndex = iEncodedEHandle & ~(iSerialNum << MAX_EDICT_BITS);
	EHANDLE h(iEntryIndex, iSerialNum);

	return h;
}

// Message handler for PyMessage
// ================== Attributes ==================
void __MsgFunc_PyNetworkVar( bf_read &msg )
{
	char buf[_MAX_PATH];
	bp::object data;

	EHANDLE h = ReadEHANLE( msg );

	msg.ReadString(buf, _MAX_PATH);

	try 
	{
		data = PyReadElement(msg);
	} 
	catch(boost::python::error_already_set &)
	{
		PyErr_Print();
		PyErr_Clear();
		return;
	}

	if( h == NULL )
	{
		if( g_debug_pynetworkvar.GetBool() )
		{
			Msg("#%d Received PyNetworkVar %s, but entity NULL\n", h.GetEntryIndex(), buf);
		}
		SrcPySystem()->AddToDelayedUpdateList( h, buf, data );
		return;
	}

	h->PyUpdateNetworkVar( buf, data );

	if( g_debug_pynetworkvar.GetBool() )
	{
		Msg("#%d Received PyNetworkVar %s\n", h->entindex(), buf);
	}
}

void __MsgFunc_PyNetworkVarChangedCallback( bf_read &msg )
{
	char buf[_MAX_PATH];
	bp::object data;

	EHANDLE h = ReadEHANLE( msg );

	msg.ReadString(buf, _MAX_PATH);

	try 
	{
		data = PyReadElement(msg);
	} 
	catch(boost::python::error_already_set &) 
	{
		PyErr_Print();
		PyErr_Clear();
		return;
	}

	if( h == NULL )
	{
		if( g_debug_pynetworkvar.GetBool() )
		{
			Msg("#%d Received PyNetworkVarCC %s, but entity NULL\n", h.GetEntryIndex(), buf);
		}
		SrcPySystem()->AddToDelayedUpdateList( h, buf, data, true );
		return;
	}

	h->PyUpdateNetworkVar( buf, data, true );

	if( g_debug_pynetworkvar.GetBool() )
	{
		Msg("#%d Received PyNetworkVarCC %s\n", h->entindex(), buf);
	}
}

// ================== Lists ==================
void __MsgFunc_PyNetworkArrayFull( bf_read &msg )
{
	char buf[_MAX_PATH];
	bp::list data;
	int length, i;

	// Read entity
	EHANDLE h = ReadEHANLE( msg );

	// Read variable name + number of elements
	msg.ReadString(buf, _MAX_PATH);
	length = msg.ReadByte();

	try 
	{
		for( i = 0; i < length; i++)
		{
			data.append( PyReadElement(msg) );
		}
	} 
	catch(boost::python::error_already_set &) 
	{
		PyErr_Print();
		PyErr_Clear();
		return;
	}

	if( h == NULL )
	{
		if( g_debug_pynetworkvar.GetBool() )
		{
			Msg("#%d Received PyNetworkArrayFull %s, but entity NULL\n", h.GetEntryIndex(), buf);
		}
		SrcPySystem()->AddToDelayedUpdateList( h, buf, data );
		return;
	}

	h->PyUpdateNetworkVar( buf, data );

	if( g_debug_pynetworkvar.GetBool() )
	{
		Msg("#%d Received PyNetworkArrayFull %s\n", h->entindex(), buf);
	}
}

void __MsgFunc_PyNetworkArrayFullChangedCallback( bf_read &msg )
{
	char buf[_MAX_PATH];
	bp::list data;
	int length, i;

	// Read entity
	EHANDLE h = ReadEHANLE( msg );

	// Read variable name + number of elements
	msg.ReadString(buf, _MAX_PATH);
	length = msg.ReadByte();

	try 
	{
		for( i = 0; i < length; i++)
		{
			data.append( PyReadElement(msg) );
		}
	} 
	catch(boost::python::error_already_set &) 
	{
		PyErr_Print();
		PyErr_Clear();
		return;
	}

	if( h == NULL )
	{
		if( g_debug_pynetworkvar.GetBool() )
		{
			Msg("#%d Received PyNetworkArrayFullCC %s, but entity NULL\n", h.GetEntryIndex(), buf);
		}
		SrcPySystem()->AddToDelayedUpdateList( h, buf, data, true );
		return;
	}

	h->PyUpdateNetworkVar( buf, data, true );

	if( g_debug_pynetworkvar.GetBool() )
	{
		Msg("#%d Received PyNetworkArrayFullCC %s\n", h->entindex(), buf);
	}
}

// ================== Dictionaries ==================
void __MsgFunc_PyNetworkDictElement( bf_read &msg )
{
	Assert(0); // Unfinished! 

	char buf[_MAX_PATH];
	bp::dict data;

	// Read entity
	EHANDLE h = ReadEHANLE( msg );

	// Read attribute name
	msg.ReadString(buf, _MAX_PATH);

	// Read single element
	try 
	{
		data[PyReadElement(msg)] = PyReadElement(msg);
	} 
	catch(boost::python::error_already_set &)
	{
		PyErr_Print();
		PyErr_Clear();
		return;
	}

	if( h == NULL )
	{
		if( g_debug_pynetworkvar.GetBool() )
		{
			Msg("#%d Received PyNetworkDictElement %s, but entity NULL\n", h.GetEntryIndex(), buf);
		}
		SrcPySystem()->AddToDelayedUpdateList( h, buf, data );
		return;
	}

	h->PyUpdateNetworkVar( buf, data );

	if( g_debug_pynetworkvar.GetBool() )
	{
		Msg("#%d Received PyNetworkDictElement %s\n", h->entindex(), buf);
	}
}

void __MsgFunc_PyNetworkDictFull( bf_read &msg )
{
	char buf[_MAX_PATH];
	bp::dict data;
	int length, i;

	// Read entity
	EHANDLE h = ReadEHANLE( msg );

	// Read variable name + number of elements
	msg.ReadString(buf, _MAX_PATH);
	length = msg.ReadByte();

	// Parse data
	try 
	{
		for( i = 0; i < length; i++)
		{
			data[PyReadElement(msg)] = PyReadElement(msg);
		}
	} 
	catch(boost::python::error_already_set &) 
	{
		PyErr_Print();
		PyErr_Clear();
		return;
	}

	if( h == NULL )
	{
		if( g_debug_pynetworkvar.GetBool() )
		{
			Msg("#%d Received PyNetworkDictFull %s, but entity NULL\n", h.GetEntryIndex(), buf);
		}
		SrcPySystem()->AddToDelayedUpdateList( h, buf, data );
		return;
	}

	h->PyUpdateNetworkVar( buf, data );

	if( g_debug_pynetworkvar.GetBool() )
	{
		Msg("#%d Received PyNetworkDictFull %s (length: %d)\n", h->entindex(), buf, length);
	}
}

void __MsgFunc_PyNetworkDictFullChangedCallback( bf_read &msg )
{
	char buf[_MAX_PATH];
	bp::dict data;
	int length, i;

	// Read entity
	EHANDLE h = ReadEHANLE( msg );

	// Read variable name + number of elements
	msg.ReadString(buf, _MAX_PATH);
	length = msg.ReadByte();

	// Parse data
	try 
	{
		for( i = 0; i < length; i++)
		{
			data[PyReadElement(msg)] = PyReadElement(msg);
		}
	} 
	catch(boost::python::error_already_set &) 
	{
		PyErr_Print();
		PyErr_Clear();
		return;
	}

	if( h == NULL )
	{
		if( g_debug_pynetworkvar.GetBool() )
		{
			Msg("#%d Received PyNetworkDictFullCC %s, but entity NULL\n", h.GetEntryIndex(), buf);
		}
		SrcPySystem()->AddToDelayedUpdateList( h, buf, data, true );
		return;
	}

	h->PyUpdateNetworkVar( buf, data, true );

	if( g_debug_pynetworkvar.GetBool() )
	{
		Msg("#%d Received PyNetworkDictFullCC %s (length: %d)\n", h->entindex(), buf, length);
	}
}

// register message handler once
void HookPyNetworkVar() 
{
	for ( int hh = 0; hh < MAX_SPLITSCREEN_PLAYERS; ++hh )
	{
		ACTIVE_SPLITSCREEN_PLAYER_GUARD( hh );
		usermessages->HookMessage( "PyNetworkVar", __MsgFunc_PyNetworkVar );
		usermessages->HookMessage( "PyNetworkVarCC", __MsgFunc_PyNetworkVarChangedCallback );
		usermessages->HookMessage( "PyNetworkArrayFull", __MsgFunc_PyNetworkArrayFull );
		usermessages->HookMessage( "PyNetworkArrayFullCC", __MsgFunc_PyNetworkArrayFullChangedCallback );
		usermessages->HookMessage( "PyNetworkDictElement", __MsgFunc_PyNetworkDictElement );
		usermessages->HookMessage( "PyNetworkDictFull", __MsgFunc_PyNetworkDictFull );
		usermessages->HookMessage( "PyNetworkDictFullCC", __MsgFunc_PyNetworkDictFullChangedCallback );
	}
}

#endif // CLIENT_DLL