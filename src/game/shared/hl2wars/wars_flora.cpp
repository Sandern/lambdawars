//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "wars_flora.h"
#include "mapentities_shared.h"
#include "gamestringpool.h"

#include "hl2wars_util_shared.h"

#ifdef CLIENT_DLL
#include "c_hl2wars_player.h"
#include "cdll_util.h"
#endif // CLIENT_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( wars_flora, CWarsFlora );

CWarsFlora::CWarsFlora()
{
#ifndef CLIENT_DLL
	AddEFlags( EFL_SERVER_ONLY );
#endif // CLIENT_DLL
}

void CWarsFlora::Spawn()
{
	BaseClass::Spawn();

	SetSolid( SOLID_NONE );
}

// Client Spawning Code
#ifdef CLIENT_DLL
bool CWarsFlora::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( FStrEq(szKeyName, "health") )
	{
		m_iHealth = Q_atoi(szValue);
	}
	else if (FStrEq(szKeyName, "spawnflags"))
	{
		m_spawnflags = Q_atoi(szValue);
	}
	else if (FStrEq(szKeyName, "model"))
	{
		SetModelName( AllocPooledString( szValue ) );
	}
	else if (FStrEq(szKeyName, "fademaxdist"))
	{
		float flFadeMaxDist = Q_atof(szValue);
		SetDistanceFade( GetMinFadeDist(), flFadeMaxDist );
	}
	else if (FStrEq(szKeyName, "fademindist"))
	{
		float flFadeMinDist = Q_atof(szValue);
		SetDistanceFade( flFadeMinDist, GetMaxFadeDist() );
	}
	else if (FStrEq(szKeyName, "fadescale"))
	{
		SetGlobalFadeScale( Q_atof(szValue) );
	}
	else if (FStrEq(szKeyName, "skin"))
	{
		SetSkin( Q_atoi(szValue) );
	}

	else
	{
		if ( !BaseClass::KeyValue( szKeyName, szValue ) )
		{
			// key hasn't been handled
			return false;
		}
	}

	return true;
}

bool CWarsFlora::Initialize()
{
	if ( InitializeAsClientEntity( STRING(GetModelName()), false ) == false )
	{
		return false;
	}

	Spawn();

	SetCollisionGroup( COLLISION_GROUP_PUSHAWAY );

	UpdatePartitionListEntry();

	CollisionProp()->UpdatePartition();

	UpdateVisibility();

	SetNextClientThink( CLIENT_THINK_NEVER );

	return true;
}

const char *CWarsFlora::ParseEntity( const char *pEntData )
{
	CEntityMapData entData( (char*)pEntData );
	char className[MAPKEY_MAXLENGTH];
	
	MDLCACHE_CRITICAL_SECTION();

	if (!entData.ExtractValue("classname", className))
	{
		Error( "classname missing from entity!\n" );
	}

	if ( !Q_strcmp( className, "wars_flora" ) )
	{
		// always force clientside entitis placed in maps
		CWarsFlora *pEntity = new CWarsFlora();

		if ( pEntity )
		{	// Set up keyvalues.
			pEntity->ParseMapData(&entData);
			
			if ( !pEntity->Initialize() )
				pEntity->Release();
		
			return entData.CurrentBufferPosition();
		}
	}

	// Just skip past all the keys.
	char keyName[MAPKEY_MAXLENGTH];
	char value[MAPKEY_MAXLENGTH];
	if ( entData.GetFirstKey(keyName, value) )
	{
		do 
		{
		} 
		while ( entData.GetNextKey(keyName, value) );
	}

	//
	// Return the current parser position in the data block
	//
	return entData.CurrentBufferPosition();
}

void CWarsFlora::SpawnMapFlora()
{
	const char *pMapData = engine->GetMapEntitiesString();
	if( !pMapData )
	{
		Warning("CWarsFlora::SpawnMapFlora: Failed to get map entities data (no map loaded?)\n");
		return;
	}

	int nEntities = 0;

	char szTokenBuffer[MAPKEY_MAXLENGTH];

	//
	//  Loop through all entities in the map data, creating each.
	//
	for ( ; true; pMapData = MapEntity_SkipToNextEntity(pMapData, szTokenBuffer) )
	{
		//
		// Parse the opening brace.
		//
		char token[MAPKEY_MAXLENGTH];
		pMapData = MapEntity_ParseToken( pMapData, token );

		//
		// Check to see if we've finished or not.
		//
		if (!pMapData)
			break;

		if (token[0] != '{')
		{
			Error( "CWarsFlora::SpawnMapFlora: found %s when expecting {", token);
			continue;
		}

		//
		// Parse the entity and add it to the spawn list.
		//

		pMapData = ParseEntity( pMapData );

		nEntities++;
	}
}

CON_COMMAND_F( wars_flora_spawn, "Spawns the specified flora model", FCVAR_CHEAT )
{
	if( args.ArgC() < 2 )
	{
		Warning("wars_flora_spawn: Not enough arguments\n");
		return;
	}

	CHL2WarsPlayer *pPlayer = CHL2WarsPlayer::GetLocalHL2WarsPlayer(); 
	if( !pPlayer )
		return;

	const MouseTraceData_t &data = pPlayer->GetMouseData();

	// Collect arguments
	const char *pModelName = args[1];

	// Load model and determine size
	const model_t *pModel = modelinfo->FindOrLoadModel( pModelName );
	if( !pModel )
	{
		Warning("wars_flora_spawn: Failed to load model\n");
		return;
	}

	Vector mins, maxs;
	modelinfo->GetModelBounds( pModel, mins, maxs );

	// Determine position
	float startradius = 0;
	float maxradius = 0.0f;
	float radiusgrow = 0.0f;
	float radiusstep = 0.0f;
	CBaseEntity *ignore = NULL;

	positioninfo_t info( data.m_vEndPos, mins, maxs, startradius, maxradius, radiusgrow, radiusstep, ignore );
	UTIL_FindPosition( info );

	if( !info.m_bSuccess )
	{
		Warning("wars_flora_spawn: Could not find a valid position\n");
		return;
	}

	// Create flora entity
	CWarsFlora *pEntity = new CWarsFlora();
	if ( !pEntity )
	{	
		Warning("wars_flora_spawn: Failed to create entity\n");
		return;
	}

	// Apply data
	QAngle angles(0, random->RandomFloat(0, 360), 0);
	pEntity->KeyValue( "model", pModelName );
	pEntity->KeyValue( "angles", VarArgs( "%f %f %f", angles.x, angles.y, angles.z ) );
	pEntity->KeyValue( "origin", VarArgs( "%f %f %f", info.m_vPosition.x, info.m_vPosition.y, info.m_vPosition.z ) );

	// Initialize
	if ( !pEntity->Initialize() )
	{	
		pEntity->Release();
		Warning("wars_flora_spawn: Failed to initialize entity\n");
		return;
	}
		
}

#endif // CLIENT_DLL