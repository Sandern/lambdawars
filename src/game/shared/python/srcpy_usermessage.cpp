//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//===========================================================================//

#include "cbase.h"
#include "srcpy.h"
#include "srcpy_usermessage.h"
#include "srcpy_entities.h"
#include <steam/steamclientpublic.h>

#ifndef CLIENT_DLL
	#include "enginecallback.h"
#else
	#include "usermessages.h"
#endif // CLIENT_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

namespace bp = boost::python;

// In python we don't want to specify the types, so we need to send the type data
// with the message. Additionally we need to ensure begin and end function is called
// This is why the message is done in one list, so we can hide everything in one function
enum PyType{
	PYTYPE_INT = 0,
	PYTYPE_FLOAT,
	PYTYPE_STRING,
	PYTYPE_BOOL,
	PYTYPE_NONE,
	PYTYPE_VECTOR,
	PYTYPE_QANGLE,
	PYTYPE_HANDLE,
	PYTYPE_LIST,
	PYTYPE_DICT,
	PYTYPE_TUPLE,
	PYTYPE_SET,
	PYTYPE_COLOR,
	PYTYPE_STEAMID,
};

#ifndef CLIENT_DLL
// TODO: Could use some optimization probably
void PyFillWriteElement( pywrite &w, bp::object data )
{
	bp::object datatype = fntype( data );

	if( datatype == builtins.attr("int") )
	{
		w.type = PYTYPE_INT;
		w.writeint = boost::python::extract<int>(data);
	}
	else if( datatype == builtins.attr("float") )
	{
		w.type = PYTYPE_FLOAT;
		w.writefloat = boost::python::extract<float>(data);
	}
	else if( datatype == builtins.attr("str") )
	{
		w.type = PYTYPE_STRING;
		w.writestr = boost::python::extract<const char *>(data);
	}
	else if( datatype == builtins.attr("bool") )
	{
		w.type = PYTYPE_BOOL;
		w.writebool = boost::python::extract<bool>(data);
	}
	else if( data == boost::python::object() )
	{
		w.type = PYTYPE_NONE;
	}
	else if( fnisinstance( data, boost::python::object( builtins.attr("list") ) ) )
	{
		w.type = PYTYPE_LIST;

		int length = boost::python::len(data);
		for( int i = 0; i < length; i++ )
		{
			pywrite write;
			PyFillWriteElement( write, data[i] );
			w.writelist.AddToTail(write);
		}
	}
	else if( datatype == builtins.attr("set") )
	{
		w.type = PYTYPE_SET;

		boost::python::object iterator = data.attr("__iter__")();

		int length = boost::python::len( data );
		for( int i = 0; i < length; i++ )
		{
			pywrite write;
			PyFillWriteElement( write, iterator.attr( PY_NEXT_METHODNAME )() );
			w.writelist.AddToTail( write );
		}
	}
	else if( datatype == builtins.attr("tuple") )
	{
		w.type = PYTYPE_TUPLE;

		int length = boost::python::len(data);
		for( int i = 0; i < length; i++ )
		{
			pywrite write;
			PyFillWriteElement( write, data[i] );
			w.writelist.AddToTail(write);
		}
	}
	else if( fnisinstance( data, boost::python::object( builtins.attr("dict") ) ) )
	{
		w.type = PYTYPE_DICT;

		bp::dict dictdata( bp::detail::borrowed_reference(data.ptr()) );

		bp::object items = dictdata.attr("items")();
		bp::object iterator = items.attr("__iter__")();
		unsigned long ulCount = bp::len(items); 
		for( unsigned long u = 0; u < ulCount; u++ )
		{
			bp::object item = iterator.attr( PY_NEXT_METHODNAME )();

			pywrite write;
			PyFillWriteElement( write, item[0] );
			w.writelist.AddToTail(write);

			pywrite write2;
			PyFillWriteElement( write2, item[1] );
			w.writelist.AddToTail(write2);
		}
	}
	else if( !V_strncmp( Py_TYPE(data.ptr())->tp_name, "Vector", 6 ) )
	{
		w.type = PYTYPE_VECTOR;
		Vector vData = boost::python::extract<Vector>(data);
		vData.CopyToArray(w.writevector);
	}
	else if( !V_strncmp( Py_TYPE(data.ptr())->tp_name, "QAngle", 6 ) )
	{
		w.type = PYTYPE_QANGLE;
		QAngle angle = boost::python::extract<QAngle>(data);
		w.writevector[0] = angle.x;
		w.writevector[1] = angle.y;
		w.writevector[2] = angle.z;
	}
	else if( !V_strncmp( Py_TYPE(data.ptr())->tp_name, "Color", 5 ) )
	{
		w.type = PYTYPE_COLOR;
		Color c = boost::python::extract<Color>(data);
		w.writecolor[0] = c.r();
		w.writecolor[1] = c.g();
		w.writecolor[2] = c.b();
		w.writecolor[3] = c.a();
	}
	else if( !V_strncmp( Py_TYPE(data.ptr())->tp_name, "CSteamID", 8 ) )
	{
		w.type = PYTYPE_STEAMID;
		CSteamID steamid = boost::python::extract<CSteamID>(data);
		w.writeuint64 = steamid.ConvertToUint64();
	}
	else
	{
		try 
		{
			CBaseHandle handle = boost::python::extract<CBaseHandle>(data);
			w.type = PYTYPE_HANDLE;
			w.writehandle = handle;
			return;
		} 
		catch( bp::error_already_set & ) 
		{
			PyErr_Clear();
		}
		try 
		{
			CBaseEntity *pEnt = boost::python::extract<CBaseEntity *>(data);
			w.type = PYTYPE_HANDLE;
			w.writehandle = pEnt;
			return;
		} 
		catch( bp::error_already_set & ) 
		{
			PyErr_Clear();
		}
		
		const char *pObjectTypeStr = bp::extract<const char *>( bp::str( datatype ) );
		const char *pObjectStr = bp::extract<const char *>( bp::str( data ) );
		char buf[512];
		V_snprintf( buf, 512, "PyFillWriteElement: Unsupported type \"%s\" for object \"%s\" in message list", pObjectTypeStr, pObjectStr );
		PyErr_SetString(PyExc_ValueError, buf );
		throw boost::python::error_already_set(); 
	}
}

void PyWriteElement( pywrite &w )
{
	WRITE_BYTE(w.type);
	switch(w.type)
	{
	case PYTYPE_INT:
		WRITE_LONG(w.writeint);
		break;
	case PYTYPE_FLOAT:
		WRITE_FLOAT(w.writefloat);
		break;
	case PYTYPE_STRING:
		WRITE_STRING(w.writestr);
		break;
	case PYTYPE_BOOL:
		WRITE_BYTE(w.writebool);
		break;
	case PYTYPE_VECTOR:
	case PYTYPE_QANGLE:
		WRITE_FLOAT(w.writevector[0]);
		WRITE_FLOAT(w.writevector[1]);
		WRITE_FLOAT(w.writevector[2]);
		break;
	case PYTYPE_COLOR:
		WRITE_BYTE(w.writecolor[0]);
		WRITE_BYTE(w.writecolor[1]);
		WRITE_BYTE(w.writecolor[2]);
		WRITE_BYTE(w.writecolor[3]);
		break;
	case PYTYPE_LIST:
	case PYTYPE_TUPLE:
	case PYTYPE_SET:
		WRITE_SHORT(w.writelist.Count());
		for( int i = 0; i < w.writelist.Count(); i++ )
			PyWriteElement( w.writelist[i] );
		break;
	case PYTYPE_DICT:
		WRITE_SHORT(w.writelist.Count()/2); // Dicts write in pairs of two (key/value)
		for( int i = 0; i < w.writelist.Count(); i++ )
			PyWriteElement( w.writelist[i] );
		break;
	case PYTYPE_STEAMID:
		WRITE_BYTES( &w.writeuint64, sizeof( w.writeuint64 ) );
		break;
	case PYTYPE_NONE:
		break;
	case PYTYPE_HANDLE:
		WRITE_EHANDLE(w.writehandle);
		break;
	}
}

void PyPrintElement( pywrite &w, int indent )
{
	for( int i = 0; i < indent; i++ )
		DevMsg(" ");

	switch(w.type)
	{
	case PYTYPE_INT:
		DevMsg("Int: %ld\n", w.writeint);
		break;
	case PYTYPE_FLOAT:
		DevMsg("Float: %f\n", w.writefloat);
		break;
	case PYTYPE_STRING:
		DevMsg("String: %s\n", w.writestr);
		break;
	case PYTYPE_BOOL:
		DevMsg("Bool: %d\n", w.writebool);
		break;
	case PYTYPE_VECTOR:
		DevMsg("Vector: %f %f %f\n", w.writevector[0],
			w.writevector[1],
			w.writevector[2] );
		break;
	case PYTYPE_QANGLE:
		DevMsg("QAngle: %f %f %f\n", w.writevector[0],
			w.writevector[1],
			w.writevector[2] );
		break;
	case PYTYPE_COLOR:
		DevMsg("Color: %d %d %d %d\n", w.writecolor[0],
			w.writecolor[1],
			w.writecolor[2],
			w.writecolor[3]);
		break;
	case PYTYPE_NONE:
		DevMsg("None\n");
		break;
	case PYTYPE_LIST:
		DevMsg("List:\n");
		for( int i = 0; i < w.writelist.Count(); i++ )
			PyPrintElement( w.writelist[i], indent + 4 );
		break;
	case PYTYPE_DICT:
		DevMsg("Dict:\n");
		for( int i = 0; i < w.writelist.Count(); i++ )
			PyPrintElement( w.writelist[i], indent + 4 );
		break;
	case PYTYPE_TUPLE:
		DevMsg("Tuple:\n");
		for( int i = 0; i < w.writelist.Count(); i++ )
			PyPrintElement( w.writelist[i], indent + 4 );
		break;
	case PYTYPE_SET:
		DevMsg("Set:\n");
		for( int i = 0; i < w.writelist.Count(); i++ )
			PyPrintElement( w.writelist[i], indent + 4 );
		break;
	case PYTYPE_HANDLE:
		DevMsg("Handle: %d\n", w.writehandle.Get());
		break;
	case PYTYPE_STEAMID:
		DevMsg("SteamID ");
		break;
	}
}

ConVar g_debug_pyusermessage("g_debug_pyusermessage", "0", FCVAR_CHEAT|FCVAR_GAMEDLL);

//-----------------------------------------------------------------------------
// Purpose: Sends a user message
//-----------------------------------------------------------------------------
void PySendUserMessage( IRecipientFilter& filter, const char *messagename, boost::python::list msg )
{
	// Skip parsing if none
	if( msg.ptr() == Py_None )
	{
		// Shortcut
		UserMessageBegin( filter, "PyMessage");
		WRITE_STRING(messagename);
		WRITE_BYTE(0);	// len(msg) == 0
		MessageEnd();
		return;
	}

	// Parse list
	int length = 0;
	CUtlVector<pywrite> writelist;
	try 
	{
		length = boost::python::len(msg);
		for( int i = 0; i < length; i++ )
		{
			pywrite write;
			PyFillWriteElement( write, boost::python::object(msg[i]) );
			writelist.AddToTail(write);
		}
	} 
	catch( boost::python::error_already_set & ) 
	{
		PyErr_Print();
		PyErr_Clear();
		return;
	}

	UserMessageBegin( filter, "PyMessage");
	WRITE_STRING(messagename);
	WRITE_BYTE(length);
	for( int i = 0; i < writelist.Count(); i++ )
	{
		PyWriteElement(writelist.Element(i));
	}
	MessageEnd();

	if( g_debug_pyusermessage.GetBool() )
	{
		DevMsg("== PySendUserMessage: Sending message to %s, consisting of %d elements\n", messagename, writelist.Count());
		for(int i=0; i<writelist.Count(); i++)
		{	
			PyPrintElement(writelist.Element(i));
		}
		DevMsg("== End PySendUserMessage\n"); 
	}
}
#else

boost::python::object PyReadElement( bf_read &msg )
{
	int type = msg.ReadByte();
	char buf[255];	// User message is max 255 bytes, so if a string it is likely smaller
	float x, y, z;
	byte r, g, b, a;
	long iEncodedEHandle;
	int iSerialNum, iEntryIndex;
	EHANDLE handle;
	int length;
	boost::python::list embeddedlist;
	boost::python::dict embeddeddict;
	uint64 steamid;

	try
	{
		switch(type)
		{
		case PYTYPE_INT:
			return boost::python::object( msg.ReadLong() );
		case PYTYPE_FLOAT:
			return boost::python::object( msg.ReadFloat() );
		case PYTYPE_STRING:
			msg.ReadString(buf, 255);
			return boost::python::object( buf );
		case PYTYPE_BOOL:
			return boost::python::object( msg.ReadByte() );
		case PYTYPE_NONE:
			return boost::python::object();
		case PYTYPE_VECTOR:
			x = msg.ReadFloat();
			y = msg.ReadFloat();
			z = msg.ReadFloat();
			return boost::python::object(Vector(x, y, z));
		case PYTYPE_QANGLE:
			x = msg.ReadFloat();
			y = msg.ReadFloat();
			z = msg.ReadFloat();
			return boost::python::object(QAngle(x, y, z));
		case PYTYPE_COLOR:
			r = msg.ReadByte();
			g = msg.ReadByte();
			b = msg.ReadByte();
			a = msg.ReadByte();
			return boost::python::object(Color(r, g, b, a));
		case PYTYPE_HANDLE:
			// Decode handle
			iEncodedEHandle = msg.ReadLong();
			iSerialNum = (iEncodedEHandle >> MAX_EDICT_BITS);
			iEntryIndex = iEncodedEHandle & ~(iSerialNum << MAX_EDICT_BITS);

			// If the entity already exists on te client, return the handle from the entity
			handle = EHANDLE( iEntryIndex, iSerialNum );
			if( handle )
				return handle->GetPyHandle();

			// If it does not exist, create a new handle
			try 
			{																
				return _entities.attr("PyHandle")( iEntryIndex, iSerialNum );							
			} 
			catch(boost::python::error_already_set &) 
			{					
				Warning("PyReadElement: Failed to create a PyHandle\n");				
				PyErr_Print();																		
				PyErr_Clear();												
			}	
			return bp::object();
		case PYTYPE_LIST:
			length = msg.ReadShort();
			for( int i = 0; i < length; i++ )
				embeddedlist.append( PyReadElement(msg) );
			return embeddedlist;
		case PYTYPE_TUPLE:
			length = msg.ReadShort();
			for( int i = 0; i < length; i++ )
				embeddedlist.append( PyReadElement(msg) );
			return bp::tuple(embeddedlist);
		case PYTYPE_SET:
			length = msg.ReadShort();
			for( int i = 0; i < length; i++ )
				embeddedlist.append( PyReadElement(msg) );
			return builtins.attr("set")(embeddedlist);
		case PYTYPE_DICT:
			length = msg.ReadShort();
			for( int i = 0; i < length; i++ )
				embeddeddict[PyReadElement(msg)] = PyReadElement(msg);
			return embeddeddict;
		case PYTYPE_STEAMID:
			msg.ReadBytes( &steamid, sizeof( steamid ) );
			return bp::object( CSteamID( steamid ) );
		default:
			Warning("PyReadElement: Unknown type %d\n", type);
			break;
		}
	}
	catch(boost::python::error_already_set &) 
	{
		Warning( "PyReadElement: failed to parse element: \n" );
		PyErr_Print();
		PyErr_Clear();
		return bp::object("<error>");
	}

	return bp::object();
}

//-----------------------------------------------------------------------------
// Purpose: Messaging
//-----------------------------------------------------------------------------
void __MsgFunc_PyMessage( bf_read &msg )
{
	// TODO: Would be nice to use a symbol/index instead of sending the full module name each time
	// The main problem is adding the symbol to the client before sending the message
	//short lookup = msg.ReadShort();
	//const char *mod_name = SrcPySystem()->GetModuleNameFromIndex(lookup);
	char messagename[128];
	msg.ReadString(messagename, 128);
	
	// Read message and add to a list
	// Then call the method function
	int i, length;
	boost::python::list recvlist;
	i = 0;
	length = msg.ReadByte();
	for( i = 0; i < length; i++)
	{
		try 
		{
			recvlist.append( PyReadElement(msg) );
		} 
		catch(boost::python::error_already_set &) 
		{
			Warning( "__MsgFunc_PyMessage failed to parse message: \n" );
			PyErr_Print();
			PyErr_Clear();
			return;
		}
	}

	SrcPySystem()->Run<const char *, boost::python::object>( SrcPySystem()->Get("_DispatchMessage", "core.usermessages", true ), messagename, recvlist );
}

// register message handler once
void HookPyMessage() 
{
	for ( int hh = 0; hh < MAX_SPLITSCREEN_PLAYERS; ++hh )
	{
		ACTIVE_SPLITSCREEN_PLAYER_GUARD( hh );
		usermessages->HookMessage( "PyMessage", __MsgFunc_PyMessage );
	}
}

#endif // CLIENT_DLL