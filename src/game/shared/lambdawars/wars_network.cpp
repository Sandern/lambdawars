//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "wars_network.h"
#include "inetchannelinfo.h"

#include "srcpy.h"

#include "wars/iwars_extension.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar wars_net_debug_send( "wars_net_debug_send", "0", FCVAR_CHEAT|FCVAR_REPLICATED );

static void WarsNet_WriteEntityDataInternal( boost::python::object data );

static bool s_EntityUpdateStarted = false;
static bool s_isLoopback = false;
static bool s_wroteData = false;
static WarsEntityUpdateMessage_t s_baseMessageData;
static CUtlBuffer s_variableMessageData;
static CSteamID s_clientSteamID;

void WarsNet_StartEntityUpdate( edict_t *pClientEdict, EHANDLE ent )
{
	if( s_EntityUpdateStarted )
	{
		Warning("WarsNet_StartEntityUpdate: entity update not properly closed\n");
		return;
	}

	s_EntityUpdateStarted = true;
	s_wroteData = false;

	// Base information
	INetChannelInfo* pNetInfo = engine->GetPlayerNetInfo( ENTINDEX( pClientEdict ) );
	s_isLoopback = pNetInfo->IsLoopback();
	s_clientSteamID = *engine->GetClientSteamID( pClientEdict );
	s_variableMessageData.Purge();
	s_baseMessageData.type = k_EMsgClient_PyEntityUpdate;
	int iSerialNum = ent.GetSerialNumber() & (1 << NUM_NETWORKED_EHANDLE_SERIAL_NUMBER_BITS) - 1;
	s_baseMessageData.iEncodedEHandle = ent.GetEntryIndex() | (iSerialNum << MAX_EDICT_BITS);

	s_variableMessageData.Put( &s_baseMessageData, sizeof( s_baseMessageData ) );
}

// Returns true if data was send or simply not needed.
// false when failed and data shouldn't be marked as transmitted.
bool WarsNet_EndEntityUpdate()
{
	if( !s_EntityUpdateStarted )
	{
		return false;
	}

	s_EntityUpdateStarted = false;

	if( !s_wroteData )
	{
		// Nothing to send
		return true;
	}

	if( wars_net_debug_send.GetBool() )
	{
		Msg("WarsNet wrote entity update of %d bytes, sending using %s\n", 
			s_variableMessageData.TellMaxPut(), s_isLoopback ? "loopback" : "Steam P2P Networking" );
	}

	if( s_isLoopback )
	{
		WarsMessageData_t *pMessageData = warsextension->InsertClientMessage();
		pMessageData->buf.Put( s_variableMessageData.Base(), s_variableMessageData.TellMaxPut() );
		return true;
	}

	return steamgameserverapicontext->SteamGameServerNetworking()->SendP2PPacket( s_clientSteamID, 
		s_variableMessageData.Base(), s_variableMessageData.TellMaxPut(), k_EP2PSendReliable, WARSNET_CLIENT_CHANNEL );
}

static void WarsNet_WriteType( unsigned char type )
{
	s_variableMessageData.Put( &type, sizeof( type ) );
}

static void WarsNet_IterateAndWriteData( boost::python::object iterableData )
{
	int length = boost::python::len( iterableData );
	boost::python::object iterator = iterableData.attr("__iter__")();

	s_variableMessageData.Put( &length, sizeof( length ) );
	for( int i = 0; i < length; i++ )
	{
		WarsNet_WriteEntityDataInternal( iterator.attr( PY_NEXT_METHODNAME )() );
	}
}

static void WarsNet_WriteEntHandle( CBaseHandle h )
{
	int iSerialNum = h.GetSerialNumber() & (1 << NUM_NETWORKED_EHANDLE_SERIAL_NUMBER_BITS) - 1;
	long encodedEHandle = h.GetEntryIndex() | (iSerialNum << MAX_EDICT_BITS);
	s_variableMessageData.Put( &encodedEHandle, sizeof( encodedEHandle ) );
}

static void WarsNet_WriteEntityDataInternal( boost::python::object data )
{
	boost::python::object datatype = fntype( data );

	if( datatype == builtins.attr("int") )
	{
		WarsNet_WriteType( WARSNET_INT );	
		int intData = boost::python::extract<int>(data);
		s_variableMessageData.Put( &intData, sizeof( intData ) );
	}
	else if( datatype == builtins.attr("float") )
	{
		WarsNet_WriteType( WARSNET_FLOAT );	
		float floatData = boost::python::extract<float>(data);
		s_variableMessageData.Put( &floatData, sizeof( floatData ) );
	}
	else if( datatype == builtins.attr("str") )
	{
		WarsNet_WriteType( WARSNET_STRING );	
		const char *strData = boost::python::extract<const char *>(data);
		int strLen = V_strlen( strData );
		s_variableMessageData.Put( &strLen, sizeof( strLen ) );
		s_variableMessageData.Put( strData, strLen );
	}
	else if( datatype == builtins.attr("bool") )
	{
		WarsNet_WriteType( WARSNET_BOOL );
		unsigned char boolData = boost::python::extract<bool>(data);
		s_variableMessageData.Put( &boolData, sizeof( boolData ) );
	}
	else if( data == boost::python::object() )
	{
		WarsNet_WriteType( WARSNET_NONE );
	}
	else if( fnisinstance( data, boost::python::object( builtins.attr("list") ) ) )
	{
		WarsNet_WriteType( WARSNET_LIST );
		WarsNet_IterateAndWriteData( data );
	}
	else if( datatype == builtins.attr("set") )
	{
		WarsNet_WriteType( WARSNET_SET );
		WarsNet_IterateAndWriteData( data );
	}
	else if( datatype == builtins.attr("tuple") )
	{
		WarsNet_WriteType( WARSNET_TUPLE );
		WarsNet_IterateAndWriteData( data );
	}
	else if( fnisinstance( data, boost::python::object( builtins.attr("dict") ) ) )
	{
		WarsNet_WriteType( WARSNET_DICT );

		boost::python::object items = data.attr("items")();
		boost::python::object iterator = items.attr("__iter__")();
		unsigned int count = boost::python::len(items);
		s_variableMessageData.Put( &count, sizeof( count ) );
		for( unsigned int u = 0; u < count; u++ )
		{
			boost::python::object item = iterator.attr( PY_NEXT_METHODNAME )();

			WarsNet_WriteEntityDataInternal( item[0] );
			WarsNet_WriteEntityDataInternal( item[1] );
		}
	}
	else if( !V_strncmp( Py_TYPE(data.ptr())->tp_name, "Vector", 6 ) )
	{
		WarsNet_WriteType( WARSNET_VECTOR );
		Vector vData = boost::python::extract<Vector>(data);
		s_variableMessageData.Put( &vData, sizeof( vData ) );
	}
	else if( !V_strncmp( Py_TYPE(data.ptr())->tp_name, "QAngle", 6 ) )
	{
		WarsNet_WriteType( WARSNET_QANGLE );
		QAngle angle = boost::python::extract<QAngle>(data);
		s_variableMessageData.Put( &angle, sizeof( angle ) );
	}
	else if( !V_strncmp( Py_TYPE(data.ptr())->tp_name, "Color", 5 ) )
	{
		WarsNet_WriteType( WARSNET_COLOR );
		Color c = boost::python::extract<Color>(data);
		s_variableMessageData.Put( &c, sizeof( c ) );
	}
	else if( !V_strncmp( Py_TYPE(data.ptr())->tp_name, "CSteamID", 8 ) )
	{
		WarsNet_WriteType( WARSNET_STEAMID );
		CSteamID steamid = boost::python::extract<CSteamID>(data);
		uint64 steamidUInt64 = steamid.ConvertToUint64();
		s_variableMessageData.Put( &steamidUInt64, sizeof( steamidUInt64 ) );
	}
	else
	{
		try 
		{
			CBaseHandle handle = boost::python::extract<CBaseHandle>(data);
			WarsNet_WriteType( WARSNET_HANDLE );
			WarsNet_WriteEntHandle( handle );
			return;
		} 
		catch( boost::python::error_already_set & ) 
		{
			PyErr_Clear();
		}
		try 
		{
			CBaseEntity *pEnt = boost::python::extract<CBaseEntity *>(data);
			WarsNet_WriteType( WARSNET_HANDLE );
			WarsNet_WriteEntHandle( EHANDLE( pEnt ) );
			return;
		} 
		catch( boost::python::error_already_set & ) 
		{
			PyErr_Clear();
		}
		
		const char *pObjectTypeStr = boost::python::extract<const char *>( boost::python::str( datatype ) );
		const char *pObjectStr = boost::python::extract<const char *>( boost::python::str( data ) );
		char buf[512];
		V_snprintf( buf, 512, "PyFillWriteElement: Unsupported type \"%s\" for object \"%s\" in message list", pObjectTypeStr, pObjectStr );
		PyErr_SetString(PyExc_ValueError, buf );
		throw boost::python::error_already_set();
	}
}

void WarsNet_WriteEntityData( const char *name, boost::python::object data, bool changecallback )
{
	s_wroteData = true;

	if( wars_net_debug_send.GetBool() )
	{
		Msg("WarsNet Writing entity variable update for %s\n", name);
	}

	// Indicate we are writing a new variable
	WarsNet_WriteType( changecallback ? WARSNET_ENTVAR_CC : WARSNET_ENTVAR );

	int strLen = V_strlen( name );
	s_variableMessageData.Put( &strLen, sizeof( strLen ) );
	s_variableMessageData.Put( name, strLen );

	// Write the actual data
	WarsNet_WriteEntityDataInternal( data );
}
