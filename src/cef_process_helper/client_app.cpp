#include "client_app.h"
#include "render_browser_helpers.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool ClientApp::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
										CefProcessId source_process,
										CefRefPtr<CefProcessMessage> message)
{
	CefRefPtr<RenderBrowser> renderBrowser = FindBrowser( browser );
	if( !renderBrowser )
	{
		SendWarning(browser, "No render process found\n");
		return false;
	}

	CefString msgname = message->GetName();

	if( msgname == "ping" ) 
	{
		CefRefPtr<CefProcessMessage> retmessage =
			CefProcessMessage::Create("pong");
		browser->SendProcessMessage(PID_BROWSER, retmessage);

		//SendMsg( browser, "Pong from render browser %d\n", browser->GetIdentifier() );
		return true;
	}
	if( msgname == "requeststats" ) 
	{
		CefRefPtr<CefProcessMessage> retmessage =
			CefProcessMessage::Create("statistics");
		browser->SendProcessMessage(PID_BROWSER, retmessage);

		//char buf[512];
		//CefRefPtr<CefListValue> args = retmessage->GetArgumentList();
		//V_snprintf( "Objects: %d", sizeof( buf ), renderBrowser->geto
		//args->SetString(0, );

		return true;
	}
	else if( msgname == "createglobalobject" ) 
	{
		CefRefPtr<CefListValue> args = message->GetArgumentList();
		CefString identifier = args->GetString( 0 );
		CefString objectName = args->GetString( 1 );

		if( !renderBrowser->CreateGlobalObject( identifier, objectName ) )
			SendWarning(browser, "Failed to create global object %ls\n", objectName.c_str());

		return true;
	}
	else if( msgname == "createfunction" ) 
	{
		CefRefPtr<CefListValue> args = message->GetArgumentList();
		CefString identifier = args->GetString( 0 );
		CefString objectName = args->GetString( 1 );
		CefString iParentIdentifier = "";
		if( args->GetType( 2 ) == VTYPE_STRING )
			iParentIdentifier = args->GetString( 2 );

		if( !renderBrowser->CreateFunction( identifier, objectName, iParentIdentifier ) )
			SendWarning(browser, "Failed to create function object %ls\n", objectName.c_str());

		return true;
	}
	else if( msgname == "createfunctionwithcallback" ) 
	{
		CefRefPtr<CefListValue> args = message->GetArgumentList();
		CefString identifier = args->GetString( 0 );
		CefString objectName = args->GetString( 1 );
		CefString iParentIdentifier = "";
		if( args->GetType( 2 ) == VTYPE_STRING )
			iParentIdentifier = args->GetString( 2 );

		if( !renderBrowser->CreateFunction( identifier, objectName, iParentIdentifier, true ) )
			SendWarning(browser, "Failed to create function with callback object %ls\n", objectName.c_str());

		return true;
	}
	else if( msgname == "callbackmethod" )
	{
		CefRefPtr<CefListValue> args = message->GetArgumentList();
		int iCallbackID = args->GetInt( 0 );
		CefRefPtr<CefListValue> methodargs = args->GetList( 1 );

		if( !renderBrowser->DoCallback( iCallbackID, methodargs ) )
			SendWarning(browser, "Failed to do callback for id %d\n", iCallbackID);

		return true;
	}
	else if( msgname == "calljswithresult" ) 
	{
		CefRefPtr<CefListValue> args = message->GetArgumentList();
		CefString identifier = args->GetString( 0 );
		CefString code = args->GetString( 1 );

		if( !renderBrowser->ExecuteJavascriptWithResult( identifier, code ) )
			SendWarning(browser, "Failed to call javascript with result: %ls\n", code.c_str());

		return true;
	}
	else if( msgname == "invoke" ) 
	{
		CefRefPtr<CefListValue> args = message->GetArgumentList();
		CefString identifier = args->GetString( 0 );
		CefString methodname = args->GetString( 1 );
		CefRefPtr<CefListValue> methodargs = args->GetList( 2 );

		if( !renderBrowser->Invoke( identifier, methodname, methodargs ) )
			SendWarning(browser, "Failed to invoke id %ls with methodname %ls\n", identifier.c_str(), methodname.c_str());

		return true;
	}
	else if( msgname == "invokewithresult" ) 
	{
		CefRefPtr<CefListValue> args = message->GetArgumentList();
		CefString iResultIdentifier = args->GetString( 0 );
		CefString identifier = args->GetString( 1 );
		CefString methodname = args->GetString( 2 );
		CefRefPtr<CefListValue> methodargs = args->GetList( 3 );

		if( !renderBrowser->InvokeWithResult( iResultIdentifier, identifier, methodname, methodargs ) )
			SendWarning(browser, "Failed to invoke with result id %ls / %ls with methodname %ls\n", iResultIdentifier.c_str(), identifier.c_str(), methodname.c_str());

		return true;
	}
	else if( msgname == "objectsetattr" ) 
	{
		CefRefPtr<CefListValue> args = message->GetArgumentList();
		CefString identifier = args->GetString( 0 );
		CefString attrname = args->GetString( 1 );

		CefRefPtr<CefV8Value> value = ListValueToV8Value( renderBrowser, args, 2 );
		if( !renderBrowser->ObjectSetAttr( identifier, attrname, value ) ) {
			SendWarning(browser, "Failed to set attribute for object with id %ls with attrname %ls\n", identifier.c_str(), attrname.c_str());
		}
	}
	else
	{
		SendWarning( browser, "Unknown proccess message %ls\n", msgname.c_str() );
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void ClientApp::OnBeforeCommandLineProcessing( const CefString& process_type, CefRefPtr<CefCommandLine> command_line )
{
	command_line->AppendSwitch( CefString( "no-proxy-server" ) );

	command_line->AppendSwitch( CefString( "disable-sync" ) );
	//command_line->AppendSwitch( CefString( "disable-extensions" ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientApp::OnBrowserCreated(CefRefPtr<CefBrowser> browser)
{
	m_Browsers.push_back( new RenderBrowser( browser, this ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientApp::OnBrowserDestroyed(CefRefPtr<CefBrowser> browser)
{
	std::vector< CefRefPtr<RenderBrowser> >::iterator i = m_Browsers.begin();
	for (; i != m_Browsers.end(); ++i)
	{
		if( (*i)->GetBrowser()->GetIdentifier() == browser->GetIdentifier() )
		{
			m_Browsers.erase( i );
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CefRefPtr<RenderBrowser> ClientApp::FindBrowser( CefRefPtr<CefBrowser> browser )
{
	std::vector< CefRefPtr<RenderBrowser> >::iterator i = m_Browsers.begin();
	for (; i != m_Browsers.end(); ++i)
	{
		if( (*i)->GetBrowser()->GetIdentifier() == browser->GetIdentifier() )
		{
			return (*i);
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientApp::OnContextCreated(CefRefPtr<CefBrowser> browser,
							CefRefPtr<CefFrame> frame,
							CefRefPtr<CefV8Context> context)
{
	if( browser->GetMainFrame()->GetIdentifier() != frame->GetIdentifier() )
	{
		SendWarning( browser, "Context created for different frame than the main frame (currently unsupported)\n");
		return;
	}

	// Tell Main process context is created
	CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create("oncontextcreated");
	browser->SendProcessMessage(PID_BROWSER, message);

	// Retrieve the context's window object.
	CefRefPtr<CefV8Value> object = context->GetGlobal();

	CefRefPtr<RenderBrowser> renderBrowser = FindBrowser( browser );
	if( !renderBrowser )
		return;

	renderBrowser->SetV8Context( context );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientApp::OnContextReleased(CefRefPtr<CefBrowser> browser,
								CefRefPtr<CefFrame> frame,
								CefRefPtr<CefV8Context> context)
{
	if( browser->GetMainFrame()->GetIdentifier() != frame->GetIdentifier() )
	{
		return;
	}

	CefRefPtr<RenderBrowser> renderBrowser = FindBrowser( browser );
	if( !renderBrowser )
		return;

	renderBrowser->SetV8Context( NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientApp::SendMsg( CefRefPtr<CefBrowser> browser, const char *pMsg, ... )
{
	va_list		argptr;
	static char		string[1024];
	
	va_start (argptr, pMsg);
	vsnprintf_s(string, sizeof(string), pMsg, argptr);
	va_end (argptr);

	CefRefPtr<CefProcessMessage> retmessage =
		CefProcessMessage::Create("msg");
	CefRefPtr<CefListValue> args = retmessage->GetArgumentList();
	args->SetString(0, string);

	browser->SendProcessMessage(PID_BROWSER, retmessage);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientApp::SendWarning( CefRefPtr<CefBrowser> browser, const char *pMsg, ... )
{
	va_list		argptr;
	static char		string[1024];
	
	va_start (argptr, pMsg);
	vsnprintf_s(string, sizeof(string), pMsg, argptr);
	va_end (argptr);

	CefRefPtr<CefProcessMessage> retmessage =
		CefProcessMessage::Create("warning");
	CefRefPtr<CefListValue> args = retmessage->GetArgumentList();
	args->SetString(0, string);

	browser->SendProcessMessage(PID_BROWSER, retmessage);
}