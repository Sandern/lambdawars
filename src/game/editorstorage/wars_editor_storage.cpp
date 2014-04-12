#include "wars_editor_storage.h"
#include "cdll_int.h"
#include "tier2/tier2_logging.h"
#include "ienginevgui.h"
#include "tier1/KeyValues.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IVEngineClient *engine = NULL;
IEngineVGui	*enginevgui = NULL;
IFileLoggingListener *filelogginglistener = NULL;
//static LoggingFileHandle_t s_TilegenLogHandle;
//DECLARE_LOGGING_CHANNEL( LOG_TilegenLayoutSystem );

//-----------------------------------------------------------------------------
// Connect, disconnect
//-----------------------------------------------------------------------------
bool CWarsEditorStorage::Connect( CreateInterfaceFn factory )
{
	if ( !BaseClass::Connect( factory ) )
		return false;

	if( !g_pFullFileSystem )
	{
		Error( "EditorStorage requires the filesystem to run!\n" );
		return false;
	}

	engine = ( IVEngineClient * )factory( VENGINE_CLIENT_INTERFACE_VERSION, NULL );
	if ( !engine )
	{
		Msg("Failed to load engine\n");
	}

	enginevgui = ( IEngineVGui * )factory( VENGINE_VGUI_VERSION, NULL );
	if ( !enginevgui )
	{
		Msg("Failed to load enginevgui\n");
	}

	if ( (filelogginglistener = (IFileLoggingListener *)factory(FILELOGGINGLISTENER_INTERFACE_VERSION, NULL)) == NULL )
		return INIT_FAILED;

	//s_TilegenLogHandle = filelogginglistener->BeginLoggingToFile( "tilegen_log.txt", "w" );
	//filelogginglistener->AssignLogChannel( LOG_TilegenLayoutSystem, s_TilegenLogHandle );
	return true;
}


void CWarsEditorStorage::Disconnect()
{
	//filelogginglistener->EndLoggingToFile( s_TilegenLogHandle );
	BaseClass::Disconnect();
}

InitReturnVal_t CWarsEditorStorage::Init()
{
	InitReturnVal_t nRetVal = BaseClass::Init();
	if ( nRetVal != INIT_OK )
		return nRetVal;

	return INIT_OK;
}

//-----------------------------------------------------------------------------
// Query interface
//-----------------------------------------------------------------------------
void *CWarsEditorStorage::QueryInterface( const char *pInterfaceName )
{
	CreateInterfaceFn factory = Sys_GetFactoryThis();	// This silly construction is necessary
	return factory( pInterfaceName, NULL );				// to prevent the LTCG compiler from crashing.
}

void CWarsEditorStorage::ClearData()
{
	for( int i = 0; i < m_hQueuedEntities.Count(); i++ )
		m_hQueuedEntities[i]->deleteThis();
	m_hQueuedEntities.Purge();
}

void CWarsEditorStorage::AddEntityToQueue( KeyValues *pEntity )
{
	m_hQueuedEntities.AddToTail( pEntity );
}

KeyValues *CWarsEditorStorage::PopEntityFromQueue()
{
	if( m_hQueuedEntities.Count() == 0 )
		return NULL;

	KeyValues *pEntity = m_hQueuedEntities.Tail();
	m_hQueuedEntities.Remove( m_hQueuedEntities.Count() - 1 );
	return pEntity;
}


EXPOSE_SINGLE_INTERFACE( CWarsEditorStorage, IWarsEditorStorage, WARS_EDITOR_STORAGE_VERSION );
