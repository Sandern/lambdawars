#include "render_browser.h"
#include "client_app.h"

static int s_NextCallbackID = 0;


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
FunctionV8Handler::FunctionV8Handler( CefRefPtr<RenderBrowser> renderBrowser ) : m_RenderBrowser(renderBrowser)
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void FunctionV8Handler::SetFunc( CefRefPtr<CefV8Value> func )
{
	m_Func = func;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool FunctionV8Handler::Execute(const CefString& name,
					CefRefPtr<CefV8Value> object,
					const CefV8ValueList& arguments,
					CefRefPtr<CefV8Value>& retval,
					CefString& exception)
{
	return m_RenderBrowser->CallFunction( m_Func, arguments, retval );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool FunctionWithCallbackV8Handler::Execute(const CefString& name,
					CefRefPtr<CefV8Value> object,
					const CefV8ValueList& arguments,
					CefRefPtr<CefV8Value>& retval,
					CefString& exception)
{
	return m_RenderBrowser->CallFunction( m_Func, arguments, retval, arguments.back() );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
RenderBrowser::RenderBrowser( CefRefPtr<CefBrowser> browser, CefRefPtr<ClientApp> clientApp ) : m_Browser(browser), m_ClientApp(clientApp)
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void RenderBrowser::SetV8Context( CefRefPtr<CefV8Context> context )
{
	m_Context = context;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void RenderBrowser::Clear()
{
	m_Context = NULL;

	m_Objects.clear();
	m_GlobalObjects.clear();
	m_Callbacks.clear();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool RenderBrowser::RegisterObject( int iIdentifier, CefRefPtr<CefV8Value> object )
{
	std::map< int, CefRefPtr<CefV8Value> >::const_iterator it = m_Objects.find( iIdentifier );
	if( it != m_Objects.end() )
		return false;

	m_Objects[iIdentifier] = object;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool RenderBrowser::CreateGlobalObject( int iIdentifier, CefString name )
{
	if( !m_Context || !m_Context->Enter() )
		return false;

	// Retrieve the context's window object.
	CefRefPtr<CefV8Value> object = m_Context->GetGlobal();

	CefRefPtr<CefV8Value> newobject = CefV8Value::CreateObject( NULL );

	// Add the "newobject" object to the "window" object.
	object->SetValue(name, newobject, V8_PROPERTY_ATTRIBUTE_NONE);

	// Remember we created this object
	if( !RegisterObject( iIdentifier, newobject ) )
		return false;
	m_GlobalObjects[name] = newobject;

	m_Context->Exit();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool RenderBrowser::CreateFunction( int iIdentifier, CefString name, int iParentIdentifier, bool bCallback )
{
	if( !m_Context || !m_Context->Enter() )
		return false;

	// Get object to bind to
	CefRefPtr<CefV8Value> object;
	if( iParentIdentifier != -1 )
	{
		std::map< int, CefRefPtr<CefV8Value> >::const_iterator it = m_Objects.find( iParentIdentifier );
		if( it != m_Objects.end() )
			object = m_Objects[iParentIdentifier];
		else
			return false;
	}
	else
	{
		object = m_Context->GetGlobal();
	}

	// Create function and bind to object
	CefRefPtr<FunctionV8Handler> funcHandler = !bCallback ? new FunctionV8Handler( this ) : new FunctionWithCallbackV8Handler( this );
	CefRefPtr<CefV8Value> func = CefV8Value::CreateFunction( name, funcHandler.get() );
	funcHandler->SetFunc( func );

	object->SetValue( name, func, V8_PROPERTY_ATTRIBUTE_NONE );

	// Register
	if( !RegisterObject( iIdentifier, func ) )
		return false;

	m_Context->Exit();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool RenderBrowser::ExecuteJavascriptWithResult( int iIdentifier, CefString code )
{
	if( !m_Context || !m_Context->Enter() )
		return false;

	// Execute code
	CefRefPtr<CefV8Value> retval;
	CefRefPtr<CefV8Exception> exception;
	if( !m_Context->Eval( code, retval, exception ) )
	{
		m_Context->Exit();
		return false;
	}

	// Register object
	if( !RegisterObject( iIdentifier, retval ) )
	{
		m_Context->Exit();
		return false;
	}

	m_Context->Exit();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Helper for converting a V8ValueList to CefList
//-----------------------------------------------------------------------------
static void V8ValueListToListValue( const CefV8ValueList& arguments, CefRefPtr<CefListValue> args )
{
	int idx = 0;

	// Add arguments
	CefV8ValueList::const_iterator i2 = arguments.begin();
	for( ; i2 != arguments.end(); ++i2 )
	{
		CefRefPtr<CefV8Value> value = (*i2);
		if( value->IsBool() )
		{
			args->SetBool( idx, value->GetBoolValue() );
		}
		else if( value->IsInt() )
		{
			args->SetInt( idx, value->GetIntValue() );
		}
		else if( value->IsDouble() )
		{
			args->SetDouble( idx, value->GetDoubleValue() );
		}
		else if( value->IsString() )
		{
			args->SetString( idx, value->GetStringValue() );
		}

		idx++;
	}
}

static CefRefPtr<CefV8Value> ListValueToV8Value( const CefRefPtr<CefListValue> args, int idx )
{
	CefRefPtr<CefV8Value> ret = NULL;

	switch( args->GetType( idx ) )
	{
		case VTYPE_INT:
		{
			ret = CefV8Value::CreateInt( args->GetInt( idx ) );
			break;
		}
		case VTYPE_DOUBLE:
		{
			ret = CefV8Value::CreateDouble( args->GetDouble( idx ) );
			break;
		}
		case VTYPE_BOOL:
		{
			ret = CefV8Value::CreateBool( args->GetBool( idx ) );
			break;
		}
		case VTYPE_STRING:
		{
			ret = CefV8Value::CreateString( args->GetString( idx ) );
			break;
		}
		case VTYPE_LIST:
		{
			CefRefPtr<CefListValue> src = args->GetList( idx );
			CefRefPtr<CefV8Value> v = CefV8Value::CreateArray( src->GetSize() );
			if( v )
			{
				for( int i = 0; i < v->GetArrayLength(); i++ )
					v->SetValue( i, ListValueToV8Value( src, i ) );
				ret = v;
			}
			break;
		}
		default:
		{
			ret = CefV8Value::CreateString( args->GetString( idx ) );
			break;
		}
	}

	return ret;
}

static void ListValueToV8ValueList( const CefRefPtr<CefListValue> args, CefV8ValueList& arguments )
{
	int idx = 0;

	arguments.clear();

	size_t n = args->GetSize();

	for( size_t i = 0; i < n; i++ )
		arguments.push_back( ListValueToV8Value( args, i ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool RenderBrowser::CallFunction(	CefRefPtr<CefV8Value> object, 
					const CefV8ValueList& arguments,
					CefRefPtr<CefV8Value>& retval, 
					CefRefPtr<CefV8Value> callback )
{
	std::map< int, CefRefPtr<CefV8Value> >::iterator i = m_Objects.begin();
	for( ; i != m_Objects.end(); ++i )
	{
		if( i->second->IsSame( object ) )
		{
			// Create message
			CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create("methodcall");
			CefRefPtr<CefListValue> args = message->GetArgumentList();

			CefRefPtr<CefListValue> methodargs = CefListValue::Create();
			V8ValueListToListValue( arguments, methodargs );

			args->SetInt( 0, i->first );
			args->SetList( 1, methodargs );

			// Store callback
			if( callback )
			{
				// Remove last, this is the callback method
				methodargs->Remove( methodargs->GetSize() - 1 );

				m_Callbacks.push_back( jscallback_t() );
				m_Callbacks.back().callback = callback;
				m_Callbacks.back().callbackid = s_NextCallbackID++;
				m_Callbacks.back().thisobject = object;

				args->SetInt( 2, m_Callbacks.back().callbackid );
			}
			else
			{
				args->SetNull( 2 );
			}

			// Send message
			m_Browser->SendProcessMessage(PID_BROWSER, message);

			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool RenderBrowser::DoCallback( int iCallbackID, CefRefPtr<CefListValue> methodargs )
{
	std::vector< jscallback_t >::iterator i = m_Callbacks.begin();
	for( ; i != m_Callbacks.end(); ++i )
	{
		if( (*i).callbackid == iCallbackID )
		{
			/*m_ClientApp->SendMsg( m_Browser, "Doing callback %d\n", iCallbackID );
			for( size_t j = 0; j < methodargs->GetSize(); j++ )
			{
				m_ClientApp->SendMsg( m_Browser, "\tArg %d: %ls\n", j, methodargs->GetString( j ).c_str() );
			}*/

			// Do callback
			if( m_Context && m_Context->Enter() )
			{
				CefV8ValueList args;
				ListValueToV8ValueList( methodargs, args );

				CefRefPtr<CefV8Value> result = (*i).callback->ExecuteFunction((*i).thisobject, args);
				if( !result )
					m_ClientApp->SendWarning( m_Browser, "Error occurred during calling callback\n");

				m_Context->Exit();
			}
			else
			{
				m_ClientApp->SendWarning( m_Browser, "No context, erasing callback...\n" );
			}

			// Remove callback
			m_Callbacks.erase( i );
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool RenderBrowser::Invoke( int iIdentifier, CefString methodname, CefRefPtr<CefListValue> methodargs )
{
	//m_ClientApp->SendMsg( m_Browser, "Invoke %d %ls\n", iIdentifier, methodname.c_str());

	// Get object
	std::map< int, CefRefPtr<CefV8Value> >::const_iterator it = m_Objects.find( iIdentifier );
	if( it == m_Objects.end() )
	{
		//m_ClientApp->SendMsg( m_Browser, "\tNo object of id %d\n", iIdentifier );
		return false;
	}

	CefRefPtr<CefV8Value> object = it->second;
	CefRefPtr<CefV8Value> method = object->GetValue( methodname );
	if( !method )
	{
		//m_ClientApp->SendMsg( m_Browser, "\tNo method of name %ls on object\n", methodname.c_str() );
		return false;
	}

	// Enter context and Make call
	if( !m_Context || !m_Context->Enter() )
		return false;

	CefV8ValueList args;
	ListValueToV8ValueList( methodargs, args );

	CefRefPtr<CefV8Value> result = method->ExecuteFunction( object, args );

	// Leave context
	m_Context->Exit();

	if( !result )
		return false;
	return true;
}