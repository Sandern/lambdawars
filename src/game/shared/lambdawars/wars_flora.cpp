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
#else
#include "hl2wars_player.h"
#endif // CLIENT_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( wars_flora, CWarsFlora );

BEGIN_DATADESC( CWarsFlora )
	DEFINE_KEYFIELD( m_bEditorManaged,		FIELD_BOOLEAN,	"editormanaged" ),
	DEFINE_KEYFIELD( m_iszIdleAnimationName,		FIELD_STRING,	"idleanimation" ),
	DEFINE_KEYFIELD( m_iszSqueezeDownAnimationName,		FIELD_STRING,	"squeezedownanimation" ),
	DEFINE_KEYFIELD( m_iszSqueezeUpAnimationName,		FIELD_STRING,	"squeezeupanimation" ),
	DEFINE_KEYFIELD( m_iszDestructionAnimationName,		FIELD_STRING,	"destructionanimation" ),
END_DATADESC()

CWarsFlora::CWarsFlora()
{
#ifndef CLIENT_DLL
	//UseClientSideAnimation();

	AddEFlags( EFL_SERVER_ONLY );
#endif // CLIENT_DLL
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWarsFlora::Precache( void )
{
	if ( GetModelName() == NULL_STRING )
	{
		Msg( "%s at (%.3f, %.3f, %.3f) has no model name!\n", GetClassname(), GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z );
		SetModelName( AllocPooledString( "models/error.mdl" ) );
	}

	PrecacheModel( STRING( GetModelName() ) );

	BaseClass::Precache();
}
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWarsFlora::Spawn()
{
#ifndef CLIENT_DLL
	Precache();
#endif // CLIENT_DLL

	BaseClass::Spawn();

#ifndef CLIENT_DLL
	SetModel( STRING( GetModelName() ) );
#endif // CLIENT_DLL

	SetSolid( SOLID_NONE );
#ifdef CLIENT_DLL
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_TRIGGER | FSOLID_NOT_SOLID );
#endif // CLIENT_DLL
	Vector vecMins = CollisionProp()->OBBMins();
	Vector vecMaxs = CollisionProp()->OBBMaxs();
	SetSize( vecMins, vecMaxs );

	SetTouch( &CWarsFlora::FloraTouch );

	m_iIdleSequence = (m_iszIdleAnimationName != NULL_STRING ? LookupSequence( STRING( m_iszIdleAnimationName ) ) : -1);
	m_iSqueezeDownSequence = (m_iszSqueezeDownAnimationName != NULL_STRING ? LookupSequence( STRING( m_iszSqueezeDownAnimationName ) ) : -1);
	m_iSqueezeUpSequence = (m_iszSqueezeUpAnimationName != NULL_STRING ? LookupSequence( STRING( m_iszSqueezeUpAnimationName ) ) : -1);
	m_iDestructSequence = (m_iszDestructionAnimationName != NULL_STRING ? LookupSequence( STRING( m_iszDestructionAnimationName ) ) : -1);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWarsFlora::FloraTouch( CBaseEntity *pOther )
{
	Msg("FloraTouch\n");
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWarsFlora::FillKeyValues( KeyValues *pEntityKey )
{
	pEntityKey->SetInt( "editormanaged", m_bEditorManaged );
	pEntityKey->SetString( "idleanimation", STRING( m_iszIdleAnimationName ) );
	pEntityKey->SetString( "squeezedownanimation", STRING( m_iszSqueezeDownAnimationName ) );
	pEntityKey->SetString( "squeezeupanimation", STRING( m_iszSqueezeUpAnimationName ) );
	pEntityKey->SetString( "destructionanimation", STRING( m_iszDestructionAnimationName ) );

	pEntityKey->SetString( "rendercolor", UTIL_VarArgs("%d %d %d", GetRenderColor().r, GetRenderColor().g, GetRenderColor().b ) );
	pEntityKey->SetInt( "spawnflags", GetSpawnFlags() );
	pEntityKey->SetInt( "skin", GetSkin() );

	return true;
}
#endif // CLIENT_DLL

// Client Spawning Code
#ifdef CLIENT_DLL
bool CWarsFlora::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( FStrEq(szKeyName, "health") )
	{
		m_iHealth = V_atoi(szValue);
	}
	else if (FStrEq(szKeyName, "spawnflags"))
	{
		m_spawnflags = V_atoi(szValue);
	}
	else if (FStrEq(szKeyName, "model"))
	{
		SetModelName( AllocPooledString( szValue ) );
	}
	else if (FStrEq(szKeyName, "fademaxdist"))
	{
		float flFadeMaxDist = V_atof(szValue);
		SetDistanceFade( GetMinFadeDist(), flFadeMaxDist );
	}
	else if (FStrEq(szKeyName, "fademindist"))
	{
		float flFadeMinDist = V_atof(szValue);
		SetDistanceFade( flFadeMinDist, GetMaxFadeDist() );
	}
	else if (FStrEq(szKeyName, "fadescale"))
	{
		SetGlobalFadeScale( V_atof(szValue) );
	}
	else if (FStrEq(szKeyName, "skin"))
	{
		SetSkin( V_atoi(szValue) );
	}
	else if (FStrEq(szKeyName, "idleanimation"))
	{
		m_iszIdleAnimationName = AllocPooledString( szValue );
	}
	else if (FStrEq(szKeyName, "squeezedownanimation"))
	{
		m_iszSqueezeDownAnimationName = AllocPooledString( szValue );
	}
	else if (FStrEq(szKeyName, "squeezeupanimation"))
	{
		m_iszSqueezeUpAnimationName = AllocPooledString( szValue );
	}
	else if (FStrEq(szKeyName, "destructionanimation"))
	{
		m_iszDestructionAnimationName = AllocPooledString( szValue );
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

	Vector mins, maxs;
	//const model_t *model = modelinfo->FindOrLoadModel(STRING(GetModelName()));
	modelinfo->GetModelBounds(GetModel(), mins, maxs);
	SetSize(mins, maxs);

	SetCollisionGroup( COLLISION_GROUP_DEBRIS );

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

	if ( !V_strcmp( className, "wars_flora" ) )
	{
		// always force clientside entitis placed in maps
		CWarsFlora *pEntity = new CWarsFlora();

		// Set up keyvalues.
		pEntity->ParseMapData(&entData);
			
		if ( !pEntity->Initialize() )
			pEntity->Release();
		
		return entData.CurrentBufferPosition();
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

#endif // CLIENT_DLL

void CWarsFlora::RemoveFloraInRadius( const Vector &vPosition, float fRadius )
{
	CUtlVector<CWarsFlora *> removeFlora;
	float fRadiusSqr = fRadius * fRadius;
#ifdef CLIENT_DLL
	for( CBaseEntity *pEntity = ClientEntityList().FirstBaseEntity(); pEntity; pEntity = ClientEntityList().NextBaseEntity( pEntity ) )
#else
	for( CBaseEntity *pEntity = gEntList.FirstEnt(); pEntity != NULL; pEntity = gEntList.NextEnt( pEntity ) )
#endif // CLIENT_DLL
	{
		CWarsFlora *pFlora = dynamic_cast<CWarsFlora *>( pEntity );
		if( pFlora && pFlora->GetAbsOrigin().DistToSqr( vPosition ) < fRadiusSqr )
		{
			removeFlora.AddToTail( pFlora );
		}
	}

	FOR_EACH_VEC( removeFlora, idx )
	{
		removeFlora[idx]->Remove();
	}
}

#ifdef CLIENT_DLL
CON_COMMAND_F( cl_wars_flora_spawn, "Spawns the specified flora model", FCVAR_CHEAT )
#else
CON_COMMAND_F( wars_flora_spawn, "Spawns the specified flora model", FCVAR_CHEAT )
#define VarArgs UTIL_VarArgs
#endif // CLIENT_DLL
{
	if( args.ArgC() < 2 )
	{
		Warning("wars_flora_spawn: Not enough arguments.\n Example usage: wars_flora_spawn <modelname> <randomradius> <editormanaged:1|0>");
		return;
	}
	
#ifdef CLIENT_DLL
	CHL2WarsPlayer *pPlayer = CHL2WarsPlayer::GetLocalHL2WarsPlayer(); 
#else
	CHL2WarsPlayer *pPlayer = ToHL2WarsPlayer( UTIL_PlayerByIndex( UTIL_GetCommandClientIndex() ) );
#endif // CLIENT_DLL
	if( !pPlayer )
		return;

	const MouseTraceData_t &data = pPlayer->GetMouseData();

	// Collect arguments
	const char *pModelName = args[1];
	float fRandomMaxRadius = args.ArgC() > 2 ? atof(args[2]) : 0;
	bool bEditorManaged = args.ArgC() > 3 ? atoi(args[3]) : false;

#ifndef CLIENT_DLL
	CBaseEntity::PrecacheModel( pModelName );
#endif // CLIENT_DLL

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

	float fRandomDegree = random->RandomFloat() * 2 * M_PI;
	float fRandomRadius = random->RandomFloat(0.0f, fRandomMaxRadius);
	Vector vRandomPosOffset( cos(fRandomDegree) * fRandomRadius, sin(fRandomDegree) * fRandomRadius, 0.0f );

	positioninfo_t info( data.m_vEndPos + vRandomPosOffset, mins, maxs, startradius, maxradius, radiusgrow, radiusstep, ignore );
	UTIL_FindPosition( info );

	if( !info.m_bSuccess )
	{
		Warning("wars_flora_spawn: Could not find a valid position\n");
		return;
	}

	// Create flora entity
	CWarsFlora *pEntity = (CWarsFlora *)CreateEntityByName("wars_flora"); //new CWarsFlora();
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
	pEntity->KeyValue( "editormanaged", bEditorManaged ? "1" : "0" );

#ifdef CLIENT_DLL
	// Initialize
	if ( !pEntity->Initialize() )
	{	
		pEntity->Release();
		Warning("wars_flora_spawn: Failed to initialize entity\n");
		return;
	}
#else
	DispatchSpawn( pEntity );
	pEntity->Activate();

	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if( !pPlayer || !pPlayer->IsConnected() )
			continue;
		engine->ClientCommand( pPlayer->edict(), VarArgs( "cl_wars_flora_spawn %s", args.ArgS() ) );
	}
#endif // CLIENT_DLL
}