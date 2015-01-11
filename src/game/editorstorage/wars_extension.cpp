#include "wars_extension.h"
#include "cdll_int.h"
#include "eiface.h"
#include "tier2/tier2_logging.h"
#include "ienginevgui.h"
#include "tier1/KeyValues.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IVEngineClient *engine = NULL;
IVEngineServer *serverengine = NULL;
IEngineVGui	*enginevgui = NULL;
IFileLoggingListener *filelogginglistener = NULL;

//-----------------------------------------------------------------------------
// Connect, disconnect
//-----------------------------------------------------------------------------
bool CWarsExtension::Connect( CreateInterfaceFn factory )
{
	if ( !BaseClass::Connect( factory ) )
		return false;

	if( !g_pFullFileSystem )
	{
		Error( "WarsExtension requires the filesystem to run!\n" );
		return false;
	}

	engine = ( IVEngineClient * )factory( VENGINE_CLIENT_INTERFACE_VERSION, NULL );
	if ( !engine )
	{
		Msg("Failed to load client engine\n");
	}

	serverengine = ( IVEngineServer * )factory( INTERFACEVERSION_VENGINESERVER, NULL );
	if ( !serverengine )
	{
		Msg("Failed to load server engine\n");
	}

	enginevgui = ( IEngineVGui * )factory( VENGINE_VGUI_VERSION, NULL );
	if ( !enginevgui )
	{
		Msg("Failed to load enginevgui\n");
	}

	if ( (filelogginglistener = (IFileLoggingListener *)factory(FILELOGGINGLISTENER_INTERFACE_VERSION, NULL)) == NULL )
		return INIT_FAILED;

	return true;
}


void CWarsExtension::Disconnect()
{
	BaseClass::Disconnect();
}

InitReturnVal_t CWarsExtension::Init()
{
	InitReturnVal_t nRetVal = BaseClass::Init();
	if ( nRetVal != INIT_OK )
		return nRetVal;

	return INIT_OK;
}

//-----------------------------------------------------------------------------
// Query interface
//-----------------------------------------------------------------------------
void *CWarsExtension::QueryInterface( const char *pInterfaceName )
{
	CreateInterfaceFn factory = Sys_GetFactoryThis();	// This silly construction is necessary
	return factory( pInterfaceName, NULL );				// to prevent the LTCG compiler from crashing.
}

void CWarsExtension::ClearData()
{
	for( int i = 0; i < m_hQueuedClientCommands.Count(); i++ )
		m_hQueuedClientCommands[i]->deleteThis();
	m_hQueuedClientCommands.Purge();

	for( int i = 0; i < m_hQueuedServerCommands.Count(); i++ )
		m_hQueuedServerCommands[i]->deleteThis();
	m_hQueuedServerCommands.Purge();
}

void CWarsExtension::QueueClientCommand( KeyValues *pCommand )
{
	m_hQueuedClientCommands.AddToTail( pCommand );
}

void CWarsExtension::QueueServerCommand( KeyValues *pCommand )
{
	m_hQueuedServerCommands.AddToTail( pCommand );
}

KeyValues *CWarsExtension::PopClientCommandQueue()
{
	if( m_hQueuedClientCommands.Count() == 0 )
		return NULL;

	KeyValues *pEntity = m_hQueuedClientCommands.Head();
	m_hQueuedClientCommands.Remove( 0 );
	return pEntity;
}

KeyValues *CWarsExtension::PopServerCommandQueue()
{
	if( m_hQueuedServerCommands.Count() == 0 )
		return NULL;

	KeyValues *pEntity = m_hQueuedServerCommands.Head();
	m_hQueuedServerCommands.Remove( 0 );
	return pEntity;
}

//-----------------------------------------------------------------------------
// Steam P2P messages
//-----------------------------------------------------------------------------
void CWarsExtension::ReceiveSteamP2PMessages( ISteamNetworking *pSteamNetworking, int channel, CUtlVector<WarsMessageData_t> &messageQueue )
{
	if( !pSteamNetworking )
		return;

	char *pchRecvBuf = NULL;
	uint32 cubMsgSize;
	CSteamID steamIDRemote;

	while ( pSteamNetworking->IsP2PPacketAvailable( &cubMsgSize, channel ) )
	{
		if ( pchRecvBuf )
			free( pchRecvBuf );

		pchRecvBuf = (char *)malloc( cubMsgSize );

		// see if there is any data waiting on the socket
		if ( !pSteamNetworking->ReadP2PPacket( pchRecvBuf, cubMsgSize, &cubMsgSize, &steamIDRemote, channel ) )
			break;

		if ( cubMsgSize < sizeof( uint32 ) )
		{
			Warning( "Got garbage on server socket, too short\n" );
			continue;
		}

		messageQueue.AddToTail();
		messageQueue.Tail().steamIDRemote = steamIDRemote;
		messageQueue.Tail().buf.Put( pchRecvBuf, cubMsgSize );
	}

	if ( pchRecvBuf )
		free( pchRecvBuf );
}

void CWarsExtension::ReceiveClientSteamP2PMessages( ISteamNetworking *pSteamNetworking )
{
	ReceiveSteamP2PMessages( pSteamNetworking, WARSNET_CLIENT_CHANNEL, m_hQueuedClientP2PMessages );
}

void CWarsExtension::ReceiveServerSteamP2PMessages( ISteamNetworking *pSteamNetworking )
{
	ReceiveSteamP2PMessages( pSteamNetworking, WARSNET_SERVER_CHANNEL, m_hQueuedServerP2PMessages );
}

WarsMessageData_t *CWarsExtension::ServerMessageHead()
{
	if( m_hQueuedServerP2PMessages.Count() == 0 )
		return NULL;
	return &m_hQueuedServerP2PMessages.Element(0);
}

bool CWarsExtension::NextServerMessage()
{
	if( m_hQueuedServerP2PMessages.Count() == 0 )
		return false;

	m_hQueuedServerP2PMessages.Remove(0);
	return m_hQueuedServerP2PMessages.Count() != 0;
}

WarsMessageData_t *CWarsExtension::InsertServerMessage()
{
	m_hQueuedServerP2PMessages.AddToTail();
	return &m_hQueuedServerP2PMessages.Tail();
}

WarsMessageData_t *CWarsExtension::ClientMessageHead()
{
	if( m_hQueuedClientP2PMessages.Count() == 0 )
		return NULL;
	return &m_hQueuedClientP2PMessages.Element(0);
}

bool CWarsExtension::NextClientMessage()
{
	if( m_hQueuedClientP2PMessages.Count() == 0 )
		return false;

	m_hQueuedClientP2PMessages.Remove(0);
	return m_hQueuedClientP2PMessages.Count() != 0;
}

WarsMessageData_t *CWarsExtension::InsertClientMessage()
{
	m_hQueuedClientP2PMessages.AddToTail();
	return &m_hQueuedClientP2PMessages.Tail();
}

EXPOSE_SINGLE_INTERFACE( CWarsExtension, IWarsExtension, WARS_EXTENSION_VERSION );
