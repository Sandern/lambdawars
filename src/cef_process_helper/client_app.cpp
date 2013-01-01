#include "client_app.h"

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
		SendWarning(browser, "No render process found");
		return false;
	}

	if( message->GetName() == "ping" ) 
	{
		CefRefPtr<CefProcessMessage> retmessage =
			CefProcessMessage::Create("pong");
		browser->SendProcessMessage(PID_BROWSER, retmessage);

		SendMsg( browser, "Just send a pong message from browser %d", browser->GetIdentifier() );
		return true;
	}
	else if( message->GetName() == "createglobalobject" ) 
	{
		CefRefPtr<CefListValue> args = message->GetArgumentList();
		int iIdentifier = args->GetInt( 0 );
		CefString objectName = args->GetString( 1 );

		if( !renderBrowser->CreateGlobalObject( iIdentifier, objectName ) )
			SendWarning(browser, "Failed to create global object %ls", objectName.c_str());
		//else
		//	SendMsg(browser, "Created global object %ls", objectName.c_str());
		return true;
	}
	else if( message->GetName() == "createfunction" ) 
	{
		CefRefPtr<CefListValue> args = message->GetArgumentList();
		int iIdentifier = args->GetInt( 0 );
		CefString objectName = args->GetString( 1 );
		int iParentIdentifier = -1;
		if( args->GetType( 2 ) == VTYPE_INT )
			iParentIdentifier = args->GetInt( 2 );

		if( !renderBrowser->CreateFunction( iIdentifier, objectName, iParentIdentifier ) )
			SendWarning(browser, "Failed to create function object %ls", objectName.c_str());
		//else
		//	SendMsg( browser, "Created function %ls", objectName.c_str());

		return true;
	}
	else if( message->GetName() == "createfunctionwithcallback" ) 
	{
		CefRefPtr<CefListValue> args = message->GetArgumentList();
		int iIdentifier = args->GetInt( 0 );
		CefString objectName = args->GetString( 1 );
		int iParentIdentifier = -1;
		if( args->GetType( 2 ) == VTYPE_INT )
			iParentIdentifier = args->GetInt( 2 );

		if( !renderBrowser->CreateFunction( iIdentifier, objectName, iParentIdentifier, true ) )
			SendWarning(browser, "Failed to create function with callback object %ls", objectName.c_str());
		//else
		//	SendMsg( browser, "Created function with callback %ls", objectName.c_str());

		return true;
	}
	else if( message->GetName() == "callbackmethod" )
	{
		CefRefPtr<CefListValue> args = message->GetArgumentList();
		int iCallbackID = args->GetInt( 0 );
		CefRefPtr<CefListValue> methodargs = args->GetList( 1 );

		if( !renderBrowser->DoCallback( iCallbackID, methodargs ) )
			SendWarning(browser, "Failed to do callback for id %d", iCallbackID);

		return true;
	}
	else if( message->GetName() == "calljswithresult" ) 
	{
		CefRefPtr<CefListValue> args = message->GetArgumentList();
		int iIdentifier = args->GetInt( 0 );
		CefString code = args->GetString( 1 );

		if( !renderBrowser->ExecuteJavascriptWithResult( iIdentifier, code ) )
			SendWarning(browser, "Failed to call javascript with result: %ls", code.c_str());

		return true;
	}
	else if( message->GetName() == "invoke" ) 
	{
		CefRefPtr<CefListValue> args = message->GetArgumentList();
		int iIdentifier = args->GetInt( 0 );
		CefString methodname = args->GetString( 1 );
		CefRefPtr<CefListValue> methodargs = args->GetList( 2 );

		if( !renderBrowser->Invoke( iIdentifier, methodname, methodargs ) )
			SendWarning(browser, "Failed to invoke id %d with methodname %ls\n", iIdentifier, methodname.c_str());

		return true;
	}
	else
	{
		SendWarning( browser, "Unknown proccess message %ls", message->GetName().c_str() );
	}

	return false;
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