//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//

#include "cbase.h"
#include "src_python_networkvar.h"
#include "src_python.h"
#include "src_python_usermessage.h"

#ifndef CLIENT_DLL
#include "enginecallback.h"
#else
#include "usermessages.h"
#endif // CLIENT_DLL

ConVar g_debug_pynetworkvar("g_debug_pynetworkvar", "0", FCVAR_CHEAT|FCVAR_REPLICATED);

#ifndef CLIENT_DLL
CPythonNetworkVarBase::CPythonNetworkVarBase( bp::object ent, const char *name, bool changedcallback )
	: m_bChangedCallback(changedcallback)
{
	Q_snprintf(m_Name, PYNETVAR_MAX_NAME, name);
	CBaseEntity *pEnt = NULL;
	try {
		pEnt = bp::extract<CBaseEntity *>(ent);
	} catch(boost::python::error_already_set &) {
		PyErr_Print();
		PyErr_Clear();
		return;
	}
	m_wrefEnt = SrcPySystem()->CreateWeakRef(ent);
	if( pEnt ) {
		pEnt->m_utlPyNetworkVars.AddToTail(this);
	}
}

CPythonNetworkVarBase::~CPythonNetworkVarBase()
{
	CBaseEntity *pEnt = NULL;
	try {
		pEnt = bp::extract<CBaseEntity *>(m_wrefEnt());
	} catch(boost::python::error_already_set &) {
		PyErr_Print();
		PyErr_Clear();
		return;
	}
	if( pEnt )
		pEnt->m_utlPyNetworkVars.FindAndRemove(this);
}

void CPythonNetworkVarBase::NetworkStateChanged( void )
{
	m_PlayerUpdateBits.SetAll();
}

void CPythonNetworkVarBase::NetworkVarsUpdateClient( CBaseEntity *pEnt, int iClient ) {}

//---------------------------------------------------------------------------------------
CPythonNetworkVar::CPythonNetworkVar( bp::object ent, const char *name, bp::object data, 
									 bool bInitStateChanged, bool changedcallback )
			: CPythonNetworkVarBase(ent, name, changedcallback)
{
	m_dataInternal = data;
	if( bInitStateChanged )
		NetworkStateChanged();
}

void CPythonNetworkVar::Set( bp::object data )
{
	if( data.ptr() == m_dataInternal.ptr() )
		return;
	m_dataInternal = data;
	NetworkStateChanged();
}

bp::object CPythonNetworkVar::Get( void )
{
	return m_dataInternal;
}

void CPythonNetworkVar::NetworkVarsUpdateClient( CBaseEntity *pEnt, int iClient )
{
	bp::object type = __builtin__.attr("type");

	pywrite write;
	try {
		PyFillWriteElement(write, Get(), type );
	} catch(boost::python::error_already_set &) {
		Warning("Failed to parse data for network variable %s:\n", m_Name );
		PyErr_Print();
		PyErr_Clear();
		Set( bp::object(0) );
		return;
	}

	CRecipientFilter filter;
	filter.MakeReliable();
	filter.AddRecipient(iClient);
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
		Msg("#%d - %f - PyNetworkVar: %s, Value -> ", pEnt->entindex(), gpGlobals->curtime, m_Name);
		PyPrintElement(write);
	}

	m_PlayerUpdateBits.Clear(iClient);
}

//---------------------------------------------------------------------------------------
CPythonNetworkArray::CPythonNetworkArray( bp::object ent, const char *name, bp::list data, 
										 bool bInitStateChanged, bool changedcallback )
: CPythonNetworkVarBase(ent, name, changedcallback)
{
	if(data.ptr() != Py_None)
	{
		m_dataInternal = data;
		if( bInitStateChanged )
			NetworkStateChanged();
	}
}

void CPythonNetworkArray::SetItem( int idx, bp::object data )
{
	if( data == boost::python::object(m_dataInternal[idx]) )
		return;
	m_dataInternal[idx] = data;
	NetworkStateChanged();
}

bp::object CPythonNetworkArray::GetItem( int idx )
{
	return m_dataInternal[idx];
}

void CPythonNetworkArray::Set( bp::list data )
{
	m_dataInternal = data;
	NetworkStateChanged();
}

void CPythonNetworkArray::NetworkVarsUpdateClient( CBaseEntity *pEnt, int iClient )
{
	bp::object type = __builtin__.attr("type");

	// Parse list
	int length = 0;
	CUtlVector<pywrite> writelist;
	try {
		length = boost::python::len(m_dataInternal);
		for(int i=0; i<length; i++)
		{
			pywrite write;
			PyFillWriteElement(write, boost::python::object(m_dataInternal[i]), type );
			writelist.AddToTail(write);
		}
	} catch(boost::python::error_already_set &) {
		PyErr_Print();
		PyErr_Clear();
		return;
	}

	CRecipientFilter filter;
	filter.MakeReliable();
	filter.AddRecipient(iClient);
	UserMessageBegin( filter, "PyNetworkArrayFull");
	WRITE_EHANDLE(pEnt);
	WRITE_STRING(m_Name);
	WRITE_BYTE(length);
	for(int i=0; i<writelist.Count(); i++)
	{
		PyWriteElement(writelist.Element(i));
	}
	MessageEnd();

	if( g_debug_pynetworkvar.GetBool() )
	{
		Msg("#%d - %f - PyNetworkArray: %s\n", pEnt->entindex(), gpGlobals->curtime, m_Name);
		for(int i=0; i<writelist.Count(); i++)
		{
			Msg("\t");
			PyPrintElement(writelist.Element(i));
		}
	}

	m_PlayerUpdateBits.Clear(iClient);
}

//---------------------------------------------------------------------------------------
CPythonNetworkDict::CPythonNetworkDict( bp::object ent, const char *name, bp::dict data, 
									   bool bInitStateChanged, bool changedcallback )
: CPythonNetworkVarBase(ent, name, changedcallback)
{
	if(data.ptr() != Py_None)
	{
		m_dataInternal = data;
		if( bInitStateChanged )
			NetworkStateChanged();
	}
}

void CPythonNetworkDict::SetItem( bp::object key, bp::object data )
{
	try {
		if( data == bp::object(m_dataInternal[key]) )
			return;
	} catch( bp::error_already_set & ) {
		PyErr_Clear();
	}

	m_dataInternal[key] = data;
	NetworkStateChanged();
}

bp::object CPythonNetworkDict::GetItem( bp::object key )
{
	return m_dataInternal[key];
}

void CPythonNetworkDict::Set( bp::dict data )
{
	m_dataInternal = data;
	NetworkStateChanged();
}

void CPythonNetworkDict::NetworkVarsUpdateClient( CBaseEntity *pEnt, int iClient )
{
	bp::object type = __builtin__.attr("type");

	// Parse list
	unsigned long length = 0;
	CUtlVector<pywrite> writelist;
	try {

		bp::object objectKey, objectValue;
		const bp::object objectKeys = m_dataInternal.iterkeys();
		const bp::object objectValues = m_dataInternal.itervalues();
		length = bp::len(m_dataInternal); 
		for( unsigned long u = 0; u < length; u++ )
		{
			objectKey = objectKeys.attr( "next" )();
			objectValue = objectValues.attr( "next" )();

			pywrite write;
			PyFillWriteElement(write, objectKey, type );
			writelist.AddToTail(write);

			pywrite write2;
			PyFillWriteElement(write2, objectValue, type );
			writelist.AddToTail(write2);
		}
	} catch(boost::python::error_already_set &) {
		PyErr_Print();
		PyErr_Clear();
		return;
	}

	CRecipientFilter filter;
	filter.MakeReliable();
	filter.AddRecipient(iClient);
	UserMessageBegin( filter, "PyNetworkDictFull");
	WRITE_EHANDLE(pEnt);
	WRITE_STRING(m_Name);
	WRITE_BYTE((int)length);
	for(int i=0; i<writelist.Count(); i++)
	{
		PyWriteElement(writelist.Element(i));
	}
	MessageEnd();

	if( g_debug_pynetworkvar.GetBool() )
	{
		Msg("#%d - %f - PyNetworkDict: %s (length: %d)\n", pEnt->entindex(), gpGlobals->curtime, m_Name, (int)length);
		for(int i=0; i<writelist.Count(); i++)
		{
			Msg("\t");
			PyPrintElement(writelist.Element(i));
		}
	}

	m_PlayerUpdateBits.Clear(iClient);
}

//---------------------------------------------------------------------------------------
void PyNetworkVarsUpdateClient( CBaseEntity *pEnt, int iEdict )
{
	if( pEnt->m_utlPyNetworkVars.Count() == 0 )
		return;

	if( pEnt->m_PyNetworkVarsPlayerTransmitBits.Get(iEdict) == false )
		return;

	for( int i=0; i<pEnt->m_utlPyNetworkVars.Count(); i++ )
	{
		if( pEnt->m_utlPyNetworkVars.Element(i)->m_PlayerUpdateBits.Get(iEdict) == false )
			continue;

		pEnt->m_utlPyNetworkVars.Element(i)->NetworkVarsUpdateClient(pEnt, iEdict);
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

	try {
		data = PyReadElement(msg);
	} catch(boost::python::error_already_set &) {
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

	try {
		data = PyReadElement(msg);
	} catch(boost::python::error_already_set &) {
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
		SrcPySystem()->AddToDelayedUpdateList( h, buf, data, true );
		return;
	}

	h->PyUpdateNetworkVar( buf, data );
	h->PyNetworkVarCallChangedCallback( buf );

	if( g_debug_pynetworkvar.GetBool() )
	{
		Msg("#%d Received PyNetworkVar %s\n", h->entindex(), buf);
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

	try {
		for( i = 0; i < length; i++)
		{
			data.append( PyReadElement(msg) );
		}
	} catch(boost::python::error_already_set &) {
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
	try {
		data[PyReadElement(msg)] = PyReadElement(msg);
	} catch(boost::python::error_already_set &) {
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
	try {
		for( i = 0; i < length; i++)
		{
			data[PyReadElement(msg)] = PyReadElement(msg);
		}
	} catch(boost::python::error_already_set &) {
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

// register message handler once
void HookPyNetworkVar() 
{
#ifdef HL2WARS_ASW_DLL
	for ( int hh = 0; hh < MAX_SPLITSCREEN_PLAYERS; ++hh )
	{
		ACTIVE_SPLITSCREEN_PLAYER_GUARD( hh );
		usermessages->HookMessage( "PyNetworkVar", __MsgFunc_PyNetworkVar );
		usermessages->HookMessage( "PyNetworkVarCC", __MsgFunc_PyNetworkVarChangedCallback );
		usermessages->HookMessage( "PyNetworkArrayFull", __MsgFunc_PyNetworkArrayFull );
		usermessages->HookMessage( "PyNetworkDictElement", __MsgFunc_PyNetworkDictElement );
		usermessages->HookMessage( "PyNetworkDictFull", __MsgFunc_PyNetworkDictFull );
	}
#else
	usermessages->HookMessage( "PyNetworkVar", __MsgFunc_PyNetworkVar );
	usermessages->HookMessage( "PyNetworkVarCC", __MsgFunc_PyNetworkVarChangedCallback );
	usermessages->HookMessage( "PyNetworkArrayFull", __MsgFunc_PyNetworkArrayFull );
	usermessages->HookMessage( "PyNetworkDictElement", __MsgFunc_PyNetworkDictElement );
	usermessages->HookMessage( "PyNetworkDictFull", __MsgFunc_PyNetworkDictFull );
#endif // HL2WARS_ASW_DEV
}

#endif // CLIENT_DLL