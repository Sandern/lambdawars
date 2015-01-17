//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "wars_network.h"
#include "wars_matchmaking.h"
#include "steam/steam_api.h"

#ifdef ENABLE_PYTHON
#include "srcpy.h"
#endif // ENABLE_PYTHON

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef ENABLE_PYTHON
ConVar wars_net_debug_receive( "wars_net_debug_receive", "0", FCVAR_CHEAT );

static boost::python::object WarsNet_ReadEntityVarData( CUtlBuffer &data, bool &success );

static boost::python::object WarsNet_ReadVector( CUtlBuffer &data )
{
	Vector vecData;
	data.Get( &vecData, sizeof( Vector ) );
	return boost::python::object( vecData );
}

static boost::python::object WarsNet_ReadQAngle( CUtlBuffer &data )
{
	QAngle vecData;
	data.Get( &vecData, sizeof( QAngle ) );
	return boost::python::object( vecData );
}

static boost::python::object WarsNet_ReadColor( CUtlBuffer &data )
{
	Color c;
	data.Get( &c, sizeof( c ) );
	return boost::python::object( c );
}

static boost::python::object WarsNet_ReadSteamID( CUtlBuffer &data )
{
	uint64 steamIDUint64;
	data.Get( &steamIDUint64, sizeof( steamIDUint64 ) );
	return boost::python::object( CSteamID( steamIDUint64 ) );
}

static boost::python::object WarsNet_ReadHandle( CUtlBuffer &data )
{
	int iSerialNum, iEntryIndex;
	long iEncodedEHandle;

	data.Get( &iEncodedEHandle, sizeof( iEncodedEHandle ) );
	iSerialNum = (iEncodedEHandle >> MAX_EDICT_BITS);
	iEntryIndex = iEncodedEHandle & ~(iSerialNum << MAX_EDICT_BITS);

	// If the entity already exists on te client, return the handle from the entity
	EHANDLE handle( iEntryIndex, iSerialNum );
	if( handle )
	{
		return handle->GetPyHandle();
	}

	// If it does not exist, create a new handle
	try 
	{																
		return _entities.attr("PyHandle")( iEntryIndex, iSerialNum );							
	} 
	catch( boost::python::error_already_set & ) 
	{					
		Warning( "WarsNet_ReadHandle: Failed to create a PyHandle!\n" );				
		PyErr_Print();																		
		PyErr_Clear();												
	}	
	return boost::python::object();
}

static boost::python::object WarsNet_ReadList( CUtlBuffer &data )
{
	bool success;
	boost::python::list l;
	int len = data.GetInt();
	for( int i = 0; i < len; i++ )
	{
		l.append( WarsNet_ReadEntityVarData( data, success ) );
	}
	return l;
}

static boost::python::object WarsNet_ReadDict( CUtlBuffer &data )
{
	bool success;
	boost::python::dict d;
	int len = data.GetInt();
	for( int i = 0; i < len; i++ )
	{
		d[ WarsNet_ReadEntityVarData( data, success ) ] = WarsNet_ReadEntityVarData( data, success );
	}
	return d;
}

static boost::python::object WarsNet_ReadString( CUtlBuffer &data )
{
	int lenStr = data.GetInt();
	char *strData = (char *)stackalloc( lenStr + 1 );
	data.Get( strData, lenStr );
	strData[lenStr] = 0;
	boost::python::object pyData = boost::python::object( (const char *)strData );
	stackfree( strData );
	return pyData;
}

static boost::python::object WarsNet_ReadEntityVarData( CUtlBuffer &data, bool &success )
{
	success = true;
	WarsNetType_e type = (WarsNetType_e)data.GetUnsignedChar();

	try
	{
		switch( type )
		{
		case WARSNET_INT:
			return boost::python::object( data.GetInt() );
		case WARSNET_FLOAT:
			return boost::python::object( data.GetFloat() );
		case WARSNET_STRING:
			return WarsNet_ReadString( data );
		case WARSNET_BOOL:
			return boost::python::object( (bool)data.GetUnsignedChar() );
		case WARSNET_NONE:
			return boost::python::object();
		case WARSNET_VECTOR:
			return WarsNet_ReadVector( data );
		case WARSNET_QANGLE:
			return WarsNet_ReadQAngle( data );
		case WARSNET_HANDLE:
			return WarsNet_ReadHandle( data );
		case WARSNET_LIST:
			return WarsNet_ReadList( data );
		case WARSNET_TUPLE:
			return boost::python::tuple( WarsNet_ReadList( data ) );
		case WARSNET_SET:
			return builtins.attr("set")( WarsNet_ReadList( data ) );
		case WARSNET_DICT:
			return WarsNet_ReadDict( data );
		case WARSNET_COLOR:
			return WarsNet_ReadColor( data );
		case WARSNET_STEAMID:
			return WarsNet_ReadSteamID( data );
		default:
			Warning("WarsNet_ReadEntityVar: Type %d not handled\n", type);
			break;
		}
	}
	catch( boost::python::error_already_set & ) 
	{
		Warning( "WarsNet_ReadEntityVarData: failed to parse data for type %d: \n", type );
		PyErr_Print();
		PyErr_Clear();
	}

	// Error state
	success = false;
	return boost::python::object();
}

static bool WarsNet_ReadEntityVar( EHANDLE &ent, CUtlBuffer &data )
{
	// Read expected start type
	WarsNetType_e entExpectedType = (WarsNetType_e)data.GetUnsignedChar();
	if( entExpectedType != WARSNET_ENTVAR && entExpectedType != WARSNET_ENTVAR_CC )
	{
		return false;
	}

	// Read variable name
	int lenName = data.GetInt();
	if( lenName == 0 )
	{
		Warning("WarsNet_ReadEntityVar: Reading wars net var, but no name written\n");
		return false;
	}
	char *varName = (char *)stackalloc( lenName + 1 );
	data.Get( varName, lenName );
	varName[lenName] = 0;

	bool success;
	boost::python::object pyData = WarsNet_ReadEntityVarData( data, success );

	if( success )
	{
		if( ent )
		{
			if( wars_net_debug_receive.GetBool() )
			{
				Msg( "#%d Received PyNetworkVar %s\n", ent->entindex(), varName );
			}
			ent->PyUpdateNetworkVar( varName, pyData, entExpectedType == WARSNET_ENTVAR_CC );
		}
		else
		{
			if( wars_net_debug_receive.GetBool() )
			{
				Msg( "#%d Received PyNetworkVar, but entity is NULL. Delaying apply data until entity exists...\n", 
					ent.GetEntryIndex(), varName );
			}
			SrcPySystem()->AddToDelayedUpdateList( ent, varName, pyData, entExpectedType == WARSNET_ENTVAR_CC );
		}
	}

	stackfree( varName );

	return success;
}

void WarsNet_ReceiveEntityUpdate( CUtlBuffer &data )
{
	if( data.TellMaxPut() <  sizeof(WarsEntityUpdateMessage_t) )
	{
		Warning("Received invalid WarsEntityUpdateMessage!\n");
		return;
	}

	int iSerialNum, iEntryIndex;

	WarsEntityUpdateMessage_t *entityUpdateMsg = (WarsEntityUpdateMessage_t *)data.Base();

	iSerialNum = (entityUpdateMsg->iEncodedEHandle >> MAX_EDICT_BITS);
	iEntryIndex = entityUpdateMsg->iEncodedEHandle & ~(iSerialNum << MAX_EDICT_BITS);

	EHANDLE handle( iEntryIndex, iSerialNum );
	data.SeekGet( CUtlBuffer::SEEK_HEAD, sizeof(WarsEntityUpdateMessage_t) );

	if( wars_net_debug_receive.GetBool() )
	{
		Msg( "WarsNet_ReceiveEntityUpdate: got entity data update of size %d bytes\n", data.TellMaxPut() );
	}

	while( WarsNet_ReadEntityVar( handle, data ) )
		continue;
}

class CWarsNet
{
public:
	CWarsNet() : m_CallbackP2PSessionRequest( this, &CWarsNet::OnP2PSessionRequest ),
		m_CallbackP2PSessionConnectFail( this, &CWarsNet::OnP2PSessionConnectFail )
	{
	}

	STEAM_CALLBACK( CWarsNet, OnP2PSessionRequest, P2PSessionRequest_t, m_CallbackP2PSessionRequest );
	STEAM_CALLBACK( CWarsNet, OnP2PSessionConnectFail, P2PSessionConnectFail_t, m_CallbackP2PSessionConnectFail );
};

static CWarsNet *s_pWarsNet = NULL;

//-----------------------------------------------------------------------------
// Purpose: Handle clients connecting
//-----------------------------------------------------------------------------
void CWarsNet::OnP2PSessionRequest( P2PSessionRequest_t *pCallback )
{
	// we'll accept a connection from anyone
	steamapicontext->SteamNetworking()->AcceptP2PSessionWithUser( pCallback->m_steamIDRemote );
}

//-----------------------------------------------------------------------------
// Purpose: Handle errors
//-----------------------------------------------------------------------------
void CWarsNet::OnP2PSessionConnectFail( P2PSessionConnectFail_t *pCallback )
{

	Warning( "CWarsNet::OnP2PSessionConnectFail: %s\n", WarsNet_TranslateP2PConnectErr( pCallback->m_eP2PSessionError ) );
}

void WarsNet_Init()
{
	s_pWarsNet = new CWarsNet();
}
void WarsNet_Shutdown()
{
	delete s_pWarsNet;
	s_pWarsNet = NULL;
}
#endif // ENABLE_PYTHON