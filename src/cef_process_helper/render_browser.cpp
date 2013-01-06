#include "render_browser.h"
#include "client_app.h"

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
	if( iParentIdentifier != INVALID_IDENTIFIER )
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
	if( !m_Context )
		return false;

	// Get object
	CefRefPtr<CefV8Value> object = NULL;

	if( iIdentifier != INVALID_IDENTIFIER )
	{
		std::map< int, CefRefPtr<CefV8Value> >::const_iterator it = m_Objects.find( iIdentifier );
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
		ListValueToV8ValueList( methodargs, args );

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
bool RenderBrowser::InvokeWithResult( int iResultIdentifier, int iIdentifier, CefString methodname, CefRefPtr<CefListValue> methodargs )
{
	if( !m_Context )
		return false;

	// Get object
	CefRefPtr<CefV8Value> object = NULL;

	if( iIdentifier != INVALID_IDENTIFIER )
	{
		std::map< int, CefRefPtr<CefV8Value> >::const_iterator it = m_Objects.find( iIdentifier );
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
		ListValueToV8ValueList( methodargs, args );

		result = method->ExecuteFunction( object, args );

		// Register result object
		RegisterObject( iResultIdentifier, result );
	}

	// Leave context
	m_Context->Exit();

	if( !result )
		return false;
	return true;
}