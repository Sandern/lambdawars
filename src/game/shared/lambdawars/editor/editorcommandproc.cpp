//====== Copyright � Sandern Corporation, All rights reserved. ===========//
//
// Purpose: Command processing for flora editor.
//
//=============================================================================//

#include "cbase.h"
#include "editorsystem.h"
#include "wars_flora.h"
#include "wars_grid.h"
#include "collisionutils.h"
#include "wars/iwars_extension.h"

#include "srcpy_navmesh.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef CLIENT_DLL
#define VarArgs UTIL_VarArgs
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEditorSystem::ProcessCommand( KeyValues *pCommand )
{
	const char *pOperation = pCommand->GetString("operation", "");

	if( V_strcmp( pOperation, "create" ) == 0 )
	{
		return ProcessCreateCommand( pCommand );
	}
	else if( V_strcmp( pOperation, "deleteflora" ) == 0 )
	{
		return ProcessDeleteFloraCommand( pCommand );
	}
	else if( V_strcmp( pOperation, "clearselection" ) == 0 )
	{
		ClearSelection();
		return true;
	}
	else if( V_strcmp( pOperation, "select" ) == 0 )
	{
		return ProcessSelectCommand( pCommand );
	}
	else if( V_strcmp( pOperation, "edit" ) == 0 )
	{
		return ProcessEditCommand( pCommand );
	}
	else if( V_strcmp( pOperation, "createcover" ) == 0 )
	{
		return ProcessCreateCoverCommand( pCommand );
	}
	else if( V_strcmp( pOperation, "destroycover" ) == 0 )
	{
		return ProcessDestroyCoverCommand( pCommand );
	}
	else if( V_strcmp( pOperation, "coverconvertoldnavmesh" ) == 0 )
	{
		WarsGrid().ConvertOldNavMeshCover();
		return true;
	}

	Warning( "CEditorSystem::ProcessCommand: unprocessed operation type \"%s\"\n", pOperation );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEditorSystem::ProcessCreateCommand( KeyValues *pCommand )
{
	const char *pClassname = pCommand->GetString("classname", "");
	KeyValues *pEntityValues = pCommand->FindKey("keyvalues");
	if( pClassname && pEntityValues )
	{
		// Annoying behavior: entity creation will fail on client if server has no precached the model yet.
#ifdef CLIENT_DLL
		const char *pModelname = pEntityValues->GetString("model", NULL);
		if( pModelname && modelinfo->GetModelIndex( pModelname ) < 0 )
		{
			//modelinfo->FindOrLoadModel( pModelname );
			warsextension->QueueClientCommand( pCommand );
			return false;
		}
#endif // CLIENT_DLL

		CBaseEntity *pEnt = CreateEntityByName( pClassname );
		if( pEnt )
		{
			FOR_EACH_VALUE( pEntityValues, pValue )
			{
				pEnt->KeyValue( pValue->GetName(), pValue->GetString() );
			}

			//KeyValuesDumpAsDevMsg( pEntityValues );

			CWarsFlora *pFlora = dynamic_cast<CWarsFlora *>( pEnt );
			if( pFlora )
			{
#ifdef CLIENT_DLL
				if( !pFlora->Initialize() )
				{
					pFlora->Release();
					pFlora = NULL;
					pEnt = NULL;
					Warning( "CEditorSystem: Failed to initialize flora entity on client with model %s\n", pEntityValues->GetString("model", "models/error.mdl") );
				}
#else
				DispatchSpawn( pFlora );
				pFlora->Activate();
#endif // CLIENT_DLL
			}
#ifdef CLIENT_DLL
			else if( !pEnt->InitializeAsClientEntity( pEntityValues->GetString("model", "models/error.mdl"), false ) )
			{
				if( pEnt->GetBaseAnimating() )
				{
					pEnt->GetBaseAnimating()->Release();
					pEnt = NULL;
				}
				Warning( "CEditorSystem: Failed to initialize entity on client with model %s\n", pEntityValues->GetString("model", "models/error.mdl") );
			}
#else
			else
			{
				DispatchSpawn( pEnt );
				pEnt->Activate();
			}
#endif // CLIENT_DLL

			// Auto select entity if specified
			if( pCommand->GetBool( "select", false ) )
			{
				Select( pEnt );
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEditorSystem::ProcessDeleteFloraCommand( KeyValues *pCommand )
{
	for ( KeyValues *pKey = pCommand->GetFirstTrueSubKey(); pKey; pKey = pKey->GetNextTrueSubKey() )
	{
		if ( V_strcmp( pKey->GetName(), "flora" ) )
			continue;

		const char *uuid = pKey->GetString("uuid", NULL);
		if( !uuid )
			continue;

		CWarsFlora::RemoveFloraByUUID( uuid );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEditorSystem::ProcessSelectCommand( KeyValues *pCommand )
{
	for ( KeyValues *pKey = pCommand->GetFirstTrueSubKey(); pKey; pKey = pKey->GetNextTrueSubKey() )
	{
		if ( V_strcmp( pKey->GetName(), "flora" ) )
			continue;

		const char *uuid = pKey->GetString("uuid", NULL);
		if( !uuid )
			continue;

		CWarsFlora *pFlora = CWarsFlora::FindFloraByUUID( uuid );
		if( !pFlora )
			continue;

		bool bDoSelect = pKey->GetBool( "select", true );
		bool bIsSelected = IsSelected( pFlora );
		if( bDoSelect && !bIsSelected )
		{
			Select( pFlora );
		}
		else if( !bDoSelect && bIsSelected )
		{
			Deselect( pFlora );
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEditorSystem::ProcessEditCommand( KeyValues *pCommand )
{
	KeyValues *pAttributes = pCommand->FindKey("keyvalues");
	if( pAttributes )
	{
		for( int i = 0; i < GetNumSelected(); i++ )
		{
			CBaseEntity *pEnt = GetSelected( i );
			if( !pEnt )
				continue;

			FOR_EACH_VALUE( pAttributes, pValue )
			{
				pEnt->KeyValue( pValue->GetName(), pValue->GetString() );

				if( V_strcmp( pValue->GetName(), "model" ) == 0 )
				{
					CBaseEntity::PrecacheModel( pValue->GetString() );
					pEnt->SetModel( pValue->GetString() );
				}
			}

			// Sequences of flora might have changed
			CWarsFlora *pFlora = dynamic_cast<CWarsFlora *>( pEnt );
			if( pFlora )
			{
				pFlora->InitFloraSequences();
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEditorSystem::ProcessCreateCoverCommand( KeyValues *pCommand )
{
	Vector vecPos;
	UTIL_StringToVector( vecPos.Base(), pCommand->GetString( "position" ) );

	WarsGrid().CreateCoverSpot( vecPos, pCommand->GetInt( "flags", 0 ) );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEditorSystem::ProcessDestroyCoverCommand( KeyValues *pCommand )
{
	Vector vecPos;
	UTIL_StringToVector( vecPos.Base(), pCommand->GetString( "position" ) );

	DestroyHidingSpot( vecPos, pCommand->GetFloat( "tolerance" ), pCommand->GetInt( "num" ), pCommand->GetInt( "excludeFlags" ) );
	//WarsGrid().DestroyCoverSpot( vecPos, pCommand->GetInt( "tolerance" ), pCommand->GetInt( "excludeFlags" ) );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEditorSystem::QueueCommand( KeyValues *pCommand )
{
	warsextension->QueueClientCommand( pCommand->MakeCopy() );
	warsextension->QueueServerCommand( pCommand->MakeCopy() );
	pCommand->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
KeyValues *CEditorSystem::CreateFloraCreateCommand( CWarsFlora *pFlora, const Vector *vOffset )
{
	KeyValues *pOperation = new KeyValues( "data" );
	pOperation->SetString("operation", "create");
	pOperation->SetString("classname", "wars_flora");

	Vector vOrigin = pFlora->GetAbsOrigin();
	if( vOffset )
	{
		vOrigin += *vOffset;
	}

	KeyValues *pEntValues = new KeyValues( "keyvalues" );
	pFlora->FillKeyValues( pEntValues );
	pEntValues->SetString( "model", STRING( pFlora->GetModelName() ) );
	pEntValues->SetString( "angles", VarArgs( "%f %f %f", pFlora->GetAbsAngles().x, pFlora->GetAbsAngles().y, pFlora->GetAbsAngles().z ) );
	pEntValues->SetString( "origin", VarArgs( "%f %f %f", vOrigin.x, vOrigin.y, vOrigin.z ) );

	pOperation->AddSubKey( pEntValues );

	return pOperation;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
KeyValues *CEditorSystem::CreateClearSelectionCommand()
{
	KeyValues *pOperation = new KeyValues( "data" );
	pOperation->SetString("operation", "clearselection");
	return pOperation;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
KeyValues *CEditorSystem::CreateEditCommand( KeyValues *pAttributes )
{
	KeyValues *pOperation = new KeyValues( "data" );
	pOperation->SetString("operation", "edit");
	pAttributes->SetName("keyvalues"); // Override name, not nice
	pOperation->AddSubKey(pAttributes);
	return pOperation;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
KeyValues *CEditorSystem::CreateCoverCreateCommand( const Vector &vPos, unsigned int flags )
{
	KeyValues *pOperation = new KeyValues( "data" );
	pOperation->SetString("operation", "createcover");
	pOperation->SetString( "position", VarArgs( "%f %f %f", vPos.x, vPos.y, vPos.z ) );
	pOperation->SetInt( "flags", flags );
	return pOperation;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
KeyValues *CEditorSystem::CreateCoverDestroyCommand( const Vector &vPos, float tolerance, int num, unsigned int excludeFlags )
{
	KeyValues *pOperation = new KeyValues( "data" );
	pOperation->SetString("operation", "destroycover");
	pOperation->SetString( "position", VarArgs( "%f %f %f", vPos.x, vPos.y, vPos.z ) );
	pOperation->SetFloat( "tolerance", tolerance );
	pOperation->SetInt( "num", num );
	pOperation->SetInt( "excludeFlags", excludeFlags );
	return pOperation;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
KeyValues *CEditorSystem::CreateCoverConvertOldNavMeshCommand()
{
	KeyValues *pOperation = new KeyValues( "data" );
	pOperation->SetString("operation", "coverconvertoldnavmesh");
	return pOperation;
}

#ifndef CLIENT_DLL
CON_COMMAND_F( cover_convert_oldnavmesh, "", 0 )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
	EditorSystem()->QueueCommand( EditorSystem()->CreateCoverConvertOldNavMeshCommand() );
}
#endif // CLIENT_DLL
