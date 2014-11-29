#include "client_app.h"
#include "render_browser.h"

#include "render_browser_helpers.h"

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
// Purpose: Called once when the browser is destroyed.
//-----------------------------------------------------------------------------
void RenderBrowser::OnDestroyed()
{
	Clear();

	m_Browser = NULL;
	m_ClientApp = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void RenderBrowser::SetV8Context( CefRefPtr<CefV8Context> context )
{
	m_Context = context;
	if( !context )
	{
		Clear();
	}
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
bool RenderBrowser::RegisterObject( CefRefPtr<CefV8Value> object, WarsCefJSObject_t &data )
{
	char uuid[37];
	WarsCef_GenerateUUID( uuid );
	m_Objects[uuid] = object;

	V_strncpy( data.uuid, uuid, sizeof( uuid ) );

	return true;
}

bool RenderBrowser::RegisterObject( CefString identifier, CefRefPtr<CefV8Value> object )
{
	m_Objects[identifier] = object;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CefRefPtr<CefV8Value> RenderBrowser::FindObjectForUUID( CefString uuid )
{
	std::map< CefString, CefRefPtr<CefV8Value> >::const_iterator it = m_Objects.find( uuid );
	if( it != m_Objects.end() )
		return it->second;
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool RenderBrowser::CreateGlobalObject( CefString identifier, CefString name )
{
	if( !m_Context || !m_Context->Enter() )
		return false;

	bool bRet = false;

	// Retrieve the context's window object.
	CefRefPtr<CefV8Value> object = m_Context->GetGlobal();

	CefRefPtr<CefV8Value> newobject = CefV8Value::CreateObject( NULL );

	// Add the "newobject" object to the "window" object.
	object->SetValue(name, newobject, V8_PROPERTY_ATTRIBUTE_NONE);

	// Remember we created this object
	if( RegisterObject( identifier, newobject ) )
	{
		m_GlobalObjects[name] = newobject;
		bRet = true;
	}


	m_Context->Exit();

	return bRet;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool RenderBrowser::CreateFunction( CefString identifier, CefString name, CefString parentIdentifier, bool bCallback )
{
	if( !m_Context || !m_Context->Enter() )
		return false;

	// Get object to bind to
	CefRefPtr<CefV8Value> object;
	if( !parentIdentifier.empty() )
	{
		std::map< CefString, CefRefPtr<CefV8Value> >::const_iterator it = m_Objects.find( parentIdentifier );
		if( it != m_Objects.end() )
		{
			object = m_Objects[parentIdentifier];
		}
		else
		{
			m_Context->Exit();
			return false;
		}
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
	if( !RegisterObject( identifier, func ) )
	{
		m_Context->Exit();
		return false;
	}

	// Set the identifier as user data on the object
	// This is needed for making method calls with the right identifier (e.g. might get multiple identifiers at some point)
	CefRefPtr<WarsCefUserData> warsUserData = new WarsCefUserData();
	warsUserData->function_uuid = identifier;
	func->SetUserData( warsUserData.get() );

	m_Context->Exit();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool RenderBrowser::ExecuteJavascriptWithResult( CefString identifier, CefString code )
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
	if( !RegisterObject( identifier, retval ) )
	{
		m_Context->Exit();
		return false;
	}

	m_Context->Exit();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool RenderBrowser::CallFunction(	CefRefPtr<CefV8Value> object, 
					const CefV8ValueList& arguments,
					CefRefPtr<CefV8Value>& retval, 
					CefRefPtr<CefV8Value> callback )
{
	CefRefPtr<CefBase> user_data = object->GetUserData();
	if( user_data )
	{
		WarsCefUserData* user_data_impl = static_cast<WarsCefUserData*>(user_data.get());

		// Create message
		CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create("methodcall");
		CefRefPtr<CefListValue> args = message->GetArgumentList();

		CefRefPtr<CefListValue> methodargs = CefListValue::Create();
		V8ValueListToListValue( this, arguments, methodargs );

		if( callback )
		{
			// Remove last, this is the callback method
			// Do this before the SetList call
			// SetList will invalidate methodargs and take ownership
			methodargs->Remove( methodargs->GetSize() - 1 );
		}

		args->SetString( 0, user_data_impl->function_uuid );
		args->SetList( 1, methodargs );

		// Store callback
		if( callback )
		{
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
	
	m_ClientApp->SendWarning( m_Browser, "Failed to call js bound function %ls\n", object->GetFunctionName().c_str() );
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
			// Do callback
			if( m_Context && m_Context->Enter() )
			{
				CefV8ValueList args;
				ListValueToV8ValueList( this, methodargs, args );

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
bool RenderBrowser::Invoke( CefString identifier, CefString methodname, CefRefPtr<CefListValue> methodargs )
{
	if( !m_Context )
		return false;

	// Get object
	CefRefPtr<CefV8Value> object = NULL;

	if( !identifier.empty() )
	{
		std::map< CefString, CefRefPtr<CefV8Value> >::const_iterator it = m_Objects.find( identifier );
		if( it == m_Objects.end() )
		{
			return false;
		}

		object = it->second;
	}

	// Enter context and Make call
	if( !m_Context->Enter() )
		return false;

	// Use global if no object was specified
	if( !object )
		object = m_Context->GetGlobal();

	CefRefPtr<CefV8Value> result = NULL;

	// Get method
	CefRefPtr<CefV8Value> method = object->GetValue( methodname );
	if( method )
	{
		// Execute method
		CefV8ValueList args;
		ListValueToV8ValueList( this, methodargs, args );

		result = method->ExecuteFunction( object, args );
	}

	// Leave context
	m_Context->Exit();

	if( !result )
		return false;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool RenderBrowser::InvokeWithResult( CefString resultIdentifier, CefString identifier, CefString methodname, CefRefPtr<CefListValue> methodargs )
{
	if( !m_Context )
		return false;

	// Get object
	CefRefPtr<CefV8Value> object = NULL;

	if( !identifier.empty() )
	{
		std::map< CefString, CefRefPtr<CefV8Value> >::const_iterator it = m_Objects.find( identifier );
		if( it == m_Objects.end() )
			return false;

		object = it->second;
	}

	// Enter context and Make call
	if( !m_Context->Enter() )
		return false;

	// Use global if no object was specified
	if( !object )
		object = m_Context->GetGlobal();

	CefRefPtr<CefV8Value> result = NULL;

	CefRefPtr<CefV8Value> method = object->GetValue( methodname );
	if( method )
	{
		// Execute method
		CefV8ValueList args;
		ListValueToV8ValueList( this, methodargs, args );

		result = method->ExecuteFunction( object, args );
		if( result )
		{
			// Register result object
			RegisterObject( resultIdentifier, result );
		}
	}

	// Leave context
	m_Context->Exit();

	if( !result )
		return false;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool RenderBrowser::ObjectSetAttr( CefString identifier, CefString attrname, CefRefPtr<CefV8Value> value )
{
	if( !m_Context )
		return false;

	// Get object
	CefRefPtr<CefV8Value> object = NULL;

	if( !identifier.empty() )
	{
		std::map< CefString, CefRefPtr<CefV8Value> >::const_iterator it = m_Objects.find( identifier );
		if( it == m_Objects.end() )
			return false;

		object = it->second;
	}

	// Enter context and Make call
	if( !m_Context->Enter() )
		return false;

	bool bRet = object->SetValue( attrname, value, V8_PROPERTY_ATTRIBUTE_NONE );

	// Leave context
	m_Context->Exit();

	return bRet;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool RenderBrowser::ObjectGetAttr( CefString identifier, CefString attrname, CefString resultIdentifier )
{
	if( !m_Context )
		return false;

	// Get object
	CefRefPtr<CefV8Value> object = NULL;

	if( !identifier.empty() )
	{
		std::map< CefString, CefRefPtr<CefV8Value> >::const_iterator it = m_Objects.find( identifier );
		if( it == m_Objects.end() )
			return false;

		object = it->second;
	}

	// Enter context and Make call
	if( !m_Context->Enter() )
		return false;

	bool bRet = false;
	CefRefPtr<CefV8Value> result = object->GetValue( attrname );
	if( result )
	{
		RegisterObject( resultIdentifier, result );
		bRet = true;
	}

	// Leave context
	m_Context->Exit();

	return bRet;
}