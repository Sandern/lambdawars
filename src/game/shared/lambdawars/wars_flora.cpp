//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "wars_flora.h"
#include "mapentities_shared.h"
#include "gamestringpool.h"
#include "worldsize.h"
#include "hl2wars_util_shared.h"
#include <filesystem.h>
#include "vstdlib/ikeyvaluessystem.h"
#include "srcpy.h"

#ifdef CLIENT_DLL
#include "c_hl2wars_player.h"
#include "cdll_util.h"
#include "playerandunitenumerator.h"
#include "c_entityflame.h"
#include "c_basetempentity.h"
#define CBaseTempEntity C_BaseTempEntity
#define CTEIgniteFlora C_TEIgniteFlora
#else
#include "hl2wars_player.h"
#endif // CLIENT_DLL

#include "editor/editorsystem.h"
#include "warseditor/iwars_editor_storage.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef CLIENT_DLL
#define VarArgs UTIL_VarArgs
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Flame/Ignite event
//-----------------------------------------------------------------------------
class CTEIgniteFlora : public CBaseTempEntity
{
public:
	
	DECLARE_CLASS( CTEIgniteFlora, CBaseTempEntity );
	DECLARE_NETWORKCLASS();

#ifndef CLIENT_DLL
	CTEIgniteFlora( const char *name )  : CBaseTempEntity( name ) {}
#else
	virtual void PostDataUpdate( DataUpdateType_t updateType );
#endif // CLIENT_DLL

	CNetworkVector( m_vecOrigin );
	CNetworkVar( float, m_fRadius );
	CNetworkVar( float, m_fLifetime );
};

#ifdef CLIENT_DLL
IMPLEMENT_CLIENTCLASS_EVENT( C_TEIgniteFlora, DT_TEIgniteFlora, CTEIgniteFlora );

BEGIN_RECV_TABLE_NOBASE( C_TEIgniteFlora, DT_TEIgniteFlora )
	RecvPropVector(RECVINFO( m_vecOrigin )),
	RecvPropFloat( RECVINFO( m_fRadius ) ),
	RecvPropFloat( RECVINFO( m_fLifetime ) ),
END_RECV_TABLE()

void CTEIgniteFlora::PostDataUpdate( DataUpdateType_t updateType )
{
	CWarsFlora::IgniteFloraInRadius( m_vecOrigin, m_fRadius, m_fLifetime );
}
#else
IMPLEMENT_SERVERCLASS_ST_NOBASE( CTEIgniteFlora, DT_TEIgniteFlora )
	SendPropVector( SENDINFO( m_vecOrigin ) ),
	SendPropFloat( SENDINFO( m_fRadius ) ),
	SendPropFloat( SENDINFO( m_fLifetime ) ),
END_SEND_TABLE()

static CTEIgniteFlora g_TEIgniteFloraEvent( "IgniteFloraEvent" );
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Flora Grid
//-----------------------------------------------------------------------------
#define FLORA_TILESIZE 256
#define FLORA_GRIDSIZE (COORD_EXTENT/FLORA_TILESIZE)
#define FLORA_KEYSINGLE( x ) (int)((MAX_COORD_INTEGER+x) / FLORA_TILESIZE)
#define FLORA_KEY( position ) (int)((MAX_COORD_INTEGER+position.x) / FLORA_TILESIZE) + ((int)((MAX_COORD_INTEGER+position.y) / FLORA_TILESIZE) * FLORA_GRIDSIZE)

typedef CUtlVector< CWarsFlora * > FloraVector;
CUtlVector< FloraVector > s_FloraGrid;

KeyValues *CWarsFlora::m_pKVFloraData = NULL;

LINK_ENTITY_TO_CLASS( wars_flora, CWarsFlora );

BEGIN_DATADESC( CWarsFlora )
	DEFINE_KEYFIELD( m_bEditorManaged,		FIELD_BOOLEAN,	"editormanaged" ),
	DEFINE_KEYFIELD( m_iszIdleAnimationName,		FIELD_STRING,	"idleanimation" ),
	DEFINE_KEYFIELD( m_iszSqueezeDownAnimationName,		FIELD_STRING,	"squeezedownanimation" ),
	DEFINE_KEYFIELD( m_iszSqueezeDownIdleAnimationName,	FIELD_STRING,	"squeezedownidleanimation" ),
	DEFINE_KEYFIELD( m_iszSqueezeUpAnimationName,		FIELD_STRING,	"squeezeupanimation" ),
	DEFINE_KEYFIELD( m_iszDestructionAnimationName,		FIELD_STRING,	"destructionanimation" ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWarsFlora::CWarsFlora() : m_iKey(-1)
{
	UseClientSideAnimation();
#ifndef CLIENT_DLL
	AddEFlags( EFL_SERVER_ONLY );
#else
	AddToClientSideAnimationList();
#endif // CLIENT_DLL

	m_iszIdleAnimationName = MAKE_STRING( "idle" );
	m_iszSqueezeDownAnimationName = MAKE_STRING( "down" );
	m_iszSqueezeDownIdleAnimationName = MAKE_STRING( "downidle" );
	m_iszSqueezeUpAnimationName = MAKE_STRING( "up" );
	m_iszDestructionAnimationName = MAKE_STRING( "kill" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWarsFlora::~CWarsFlora()
{
	RemoveFromFloraGrid();
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
	InsertInFloraGrid();

#ifndef CLIENT_DLL
	Precache();
#endif // CLIENT_DLL

	BaseClass::Spawn();

#ifndef CLIENT_DLL
	SetModel( STRING( GetModelName() ) );
#endif // CLIENT_DLL

	InitFloraData();

	SetSolid( SOLID_NONE );
	Vector vecMins = CollisionProp()->OBBMins();
	Vector vecMaxs = CollisionProp()->OBBMaxs();
	SetSize( vecMins, vecMaxs );

	m_iIdleSequence = (m_iszIdleAnimationName != NULL_STRING ? LookupSequence( STRING( m_iszIdleAnimationName ) ) : -1);
	m_iSqueezeDownSequence = (m_iszSqueezeDownAnimationName != NULL_STRING ? LookupSequence( STRING( m_iszSqueezeDownAnimationName ) ) : -1);
	m_iSqueezeDownIdleSequence = (m_iszSqueezeDownIdleAnimationName != NULL_STRING ? LookupSequence( STRING( m_iszSqueezeDownIdleAnimationName ) ) : -1);
	m_iSqueezeUpSequence = (m_iszSqueezeUpAnimationName != NULL_STRING ? LookupSequence( STRING( m_iszSqueezeUpAnimationName ) ) : -1);
	m_iDestructSequence = (m_iszDestructionAnimationName != NULL_STRING ? LookupSequence( STRING( m_iszDestructionAnimationName ) ) : -1);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWarsFlora::InitFloraData()
{
	static int keyIdle = KeyValuesSystem()->GetSymbolForString( "idlesequence" );
	static int keySqueezeDown = KeyValuesSystem()->GetSymbolForString( "squeezedownsequence" );
	static int keySqueezeDownIdle = KeyValuesSystem()->GetSymbolForString( "squeezedownidlesequence" );
	static int keySqueezeUp = KeyValuesSystem()->GetSymbolForString( "squeezeupsequence" );
	static int keyDestruct = KeyValuesSystem()->GetSymbolForString( "destructsequence" );
	static int keyIgnite = KeyValuesSystem()->GetSymbolForString( "ignite" );

	static int keyMinDist = KeyValuesSystem()->GetSymbolForString( "fademindist" );
	static int keyMaxDist = KeyValuesSystem()->GetSymbolForString( "fademaxdist" );

	// Find our section. No section? nothing to apply!
#ifdef CLIENT_DLL
	const char *pModelName = modelinfo->GetModelName( GetModel() );
#else
	const char *pModelName = STRING( GetModelName() );
#endif // CLIENT_DLL
	KeyValues *pSection = m_pKVFloraData->FindKey( pModelName );
	if ( !pSection )
	{
		return;
	}

	// Can be put on fire?
	m_bCanBeIgnited = pSection->GetBool( keyIgnite, false );

	// Sequence label names
	const char *pIdleAnimationName = pSection->GetString( keyIdle, NULL );
	if( pIdleAnimationName )
		m_iszIdleAnimationName = AllocPooledString( pIdleAnimationName );

	const char *pDownAnimationName = pSection->GetString( keySqueezeDown, NULL );
	if( pDownAnimationName )
		m_iszSqueezeDownAnimationName = AllocPooledString( pDownAnimationName );

	const char *pDownIdleAnimationName = pSection->GetString( keySqueezeDownIdle, NULL );
	if( pDownIdleAnimationName )
		m_iszSqueezeDownIdleAnimationName = AllocPooledString( pDownIdleAnimationName );

	const char *pUpAnimationName = pSection->GetString( keySqueezeUp, NULL );
	if( pUpAnimationName )
		m_iszSqueezeUpAnimationName = AllocPooledString( pUpAnimationName );

	const char *pDestructAnimationName = pSection->GetString( keyDestruct, NULL );
	if( pDestructAnimationName )
		m_iszDestructionAnimationName = AllocPooledString( pDestructAnimationName );

#ifdef CLIENT_DLL
	// Fade distance
	float fademindist = pSection->GetFloat( keyMinDist, GetMinFadeDist() );
	float fademaxdist = pSection->GetFloat( keyMaxDist, GetMaxFadeDist() );
	SetDistanceFade( fademindist, fademaxdist );
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWarsFlora::PlayDestructionAnimation()
{
	if( m_iDestructSequence == -1 )
		return;

	SetSequence( m_iDestructSequence );
	ResetSequenceInfo();
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
	pEntityKey->SetString( "squeezedownidleanimation", STRING( m_iszSqueezeDownIdleAnimationName ) );
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
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWarsFlora::UpdateUnitAvoid()
{
	if( m_iSqueezeDownSequence == -1 )
		return;

	float flRadius = CollisionProp()->BoundingRadius();
	CUnitEnumerator avoid( flRadius, GetAbsOrigin() );
	partition->EnumerateElementsInSphere( PARTITION_CLIENT_SOLID_EDICTS, GetAbsOrigin(), flRadius, false, &avoid );

	// Okay, decide how to avoid if there's anything close by
	int c = avoid.GetObjectCount();
	if( c > 0 )
	{
		int iSequence = GetSequence();
		if( iSequence != m_iSqueezeDownSequence && iSequence != m_iSqueezeDownIdleSequence  )
		{
			SetSequence( m_iSqueezeDownSequence );
			ResetSequenceInfo();
		}
		else if( iSequence == m_iSqueezeDownSequence && IsSequenceFinished() )
		{
			SetSequence( m_iSqueezeDownIdleSequence );
			ResetSequenceInfo();
		}

		m_fAvoidTimeOut = gpGlobals->curtime + 0.1f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWarsFlora::UpdateClientSideAnimation()
{
	BaseClass::UpdateClientSideAnimation();

	UpdateUnitAvoid();

	if( m_fAvoidTimeOut < gpGlobals->curtime )
	{
		int iSequence = GetSequence();
		if( iSequence == m_iSqueezeDownSequence || iSequence == m_iSqueezeDownIdleSequence )
		{
			SetSequence( m_iSqueezeUpSequence );
			ResetSequenceInfo();
		}
		else if( iSequence == m_iSqueezeUpSequence && IsSequenceFinished() )
		{
			SetSequence( m_iIdleSequence );
			ResetSequenceInfo();
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWarsFlora::Ignite( float flFlameLifetime, float flSize )
{
	if( !m_bCanBeIgnited )
		return;

	// Crash on trying attach particle flames to hitboxes if they don't exist...
	if( GetHitboxSetCount() == 0 )
		return;

	if( IsOnFire() )
		return;

	C_EntityFlame *pFlame = C_EntityFlame::Create( this, flFlameLifetime, flSize );
	AddFlag( FL_ONFIRE );
	SetEffectEntity( pFlame );
}

void CWarsFlora::IgniteLifetime( float flFlameLifetime )
{
	if( !m_bCanBeIgnited )
		return;

	if( !IsOnFire() )
		Ignite( 30, 0.0f );

	C_EntityFlame *pFlame = dynamic_cast<C_EntityFlame*>( GetEffectEntity() );

	if ( !pFlame )
		return;

	pFlame->SetLifetime( flFlameLifetime );
}

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
	else if (FStrEq(szKeyName, "squeezedownidleanimation"))
	{
		m_iszSqueezeDownIdleAnimationName = AllocPooledString( szValue );
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

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWarsFlora::Initialize()
{
	if ( InitializeAsClientEntity( STRING(GetModelName()), false ) == false )
	{
		return false;
	}

	Spawn();

	const model_t *model = GetModel();
	if( model )
	{
		Vector mins, maxs;
		modelinfo->GetModelBounds(GetModel(), mins, maxs);
		SetSize(mins, maxs);
	}

	SetCollisionGroup( COLLISION_GROUP_DEBRIS );

	UpdatePartitionListEntry();

	CollisionProp()->UpdatePartition();

	UpdateVisibility();

	SetNextClientThink( CLIENT_THINK_NEVER );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWarsFlora::InitFloraGrid()
{
	s_FloraGrid.SetCount( FLORA_TILESIZE * FLORA_TILESIZE );

	InitFloraDataKeyValues();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWarsFlora::DestroyFloraGrid()
{
	s_FloraGrid.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWarsFlora::InsertInFloraGrid()
{
	m_iKey = FLORA_KEY( GetAbsOrigin() );
	if( m_iKey < 0 || m_iKey > s_FloraGrid.Count() )
	{
		Warning("InsertInFloraGrid: Could not insert flora entity with key %d\n", m_iKey );
		m_iKey = -1;
		return;
	}
	s_FloraGrid[m_iKey].AddToTail( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWarsFlora::RemoveFromFloraGrid()
{
	if( m_iKey == -1 )
		return;

	s_FloraGrid[m_iKey].FindAndRemove( this );
	m_iKey = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWarsFlora::InitFloraDataKeyValues()
{
	if( m_pKVFloraData )
	{
		m_pKVFloraData->deleteThis();
		m_pKVFloraData = NULL;
	}

	FileFindHandle_t findHandle;
	const char *floraFilename = filesystem->FindFirstEx( "scripts/flora/*.txt", NULL, &findHandle );
	while ( floraFilename )
	{
		char floraPath[MAX_PATH];
		V_snprintf( floraPath, sizeof(floraPath), "scripts/flora/%s", floraFilename );
		//DevMsg("Found Flora data file: %s\n", floraPath);

		if( !m_pKVFloraData )
		{
			m_pKVFloraData = new KeyValues( "FloraDataFile" );
			if ( !m_pKVFloraData->LoadFromFile( filesystem, floraPath ) )
			{
				Warning( "InitFlora: Could not load flora data file %s\n", floraPath );
				m_pKVFloraData->deleteThis();
				m_pKVFloraData = NULL;
				return;
			}
		}
		else
		{
			KeyValues *pToMergeKV = new KeyValues( "FloraDataFile" );
			if ( pToMergeKV->LoadFromFile( filesystem, floraPath ) )
			{
				m_pKVFloraData->MergeFrom( pToMergeKV );
				return;
			}
			else
			{
				Warning( "InitFlora: Could not load flora data file %s\n", floraPath );
			}
			pToMergeKV->deleteThis();
			pToMergeKV = NULL;
		}

		floraFilename = filesystem->FindNext( findHandle );
	}
	filesystem->FindClose( findHandle );
}

//-----------------------------------------------------------------------------
// Purpose: Apply the functor to all flora in radius.
//-----------------------------------------------------------------------------
template < typename Functor >
bool ForAllFloraInRadius( Functor &func, const Vector &vPosition, float fRadius )
{
	CUtlVector<CWarsFlora *> foundFlora;
	float fRadiusSqr = fRadius * fRadius;

	int originX = FLORA_KEYSINGLE( vPosition.x );
	int originY = FLORA_KEYSINGLE( vPosition.y );
	int shiftLimit = ceil( fRadius / FLORA_TILESIZE );

	for( int x = originX - shiftLimit; x <= originX + shiftLimit; ++x )
	{
		if ( x < 0 || x >= FLORA_GRIDSIZE )
			continue;

		for( int y = originY - shiftLimit; y <= originY + shiftLimit; ++y )
		{
			if ( y < 0 || y >= FLORA_GRIDSIZE )
				continue;

			const FloraVector &floraVector = s_FloraGrid[x + (y * FLORA_GRIDSIZE)];
			for( int i = 0; i < floraVector.Count(); i++ )
			{
				CWarsFlora *pFlora = floraVector[i];
				if( pFlora && pFlora->GetAbsOrigin().DistToSqr( vPosition ) < fRadiusSqr )
				{
					foundFlora.AddToTail( pFlora );
				}
			}
		}
	}

	FOR_EACH_VEC( foundFlora, idx )
	{
		CWarsFlora *pFlora = foundFlora[idx];
		if( func( pFlora ) == false )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Spawn function mainly intended for editor or ingame testing
//			The editor mode, the flora entity gets synced to the editing client
//-----------------------------------------------------------------------------
bool CWarsFlora::SpawnFlora( const char *pModelname, const Vector &vPosition, const QAngle &vAngle )
{
	bool bEditorManaged = EditorSystem()->IsActive();

	// Create flora entity
	CWarsFlora *pEntity = (CWarsFlora *)CreateEntityByName("wars_flora"); //new CWarsFlora();
	if ( !pEntity )
	{	
		Warning("wars_flora_spawn: Failed to create entity\n");
		return false;
	}

	// Apply data
	pEntity->KeyValue( "model", pModelname );
	pEntity->KeyValue( "angles", VarArgs( "%f %f %f", vAngle.x, vAngle.y, vAngle.z ) );
	pEntity->KeyValue( "origin", VarArgs( "%f %f %f", vPosition.x, vPosition.y, vPosition.z ) );
	pEntity->KeyValue( "editormanaged", bEditorManaged ? "1" : "0" );

#ifdef CLIENT_DLL
	// Initialize
	if ( !pEntity->Initialize() )
	{	
		pEntity->Release();
		Warning("CWarsFlora::SpawnFlora: Failed to initialize entity\n");
		return false;
	}
#else
	DispatchSpawn( pEntity );
	pEntity->Activate();

	if( bEditorManaged )
	{
		KeyValues *pOperation = new KeyValues( "data" );
		pOperation->SetString("operation", "create");
		pOperation->SetString("classname", "wars_flora");

		KeyValues *pEntValues = new KeyValues( "keyvalues" );
		pEntity->FillKeyValues( pEntValues );
		pEntValues->SetString( "model", pModelname );
		pEntValues->SetString( "angles", VarArgs( "%f %f %f", vAngle.x, vAngle.y, vAngle.z ) );
		pEntValues->SetString( "origin", VarArgs( "%f %f %f", vPosition.x, vPosition.y, vPosition.z ) );

		pOperation->AddSubKey( pEntValues );

		warseditorstorage->AddEntityToQueue( pOperation );
	}
#endif // CLIENT_DLL

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Removes flora in a radius. In Editor mode, this gets synced from
//		    server to client for the editing player.
//-----------------------------------------------------------------------------
void CWarsFlora::RemoveFloraInRadius( const Vector &vPosition, float fRadius, int nMax )
{
	int iCount = 0;
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
			iCount++;
			if( nMax != -1 && iCount >= nMax )
				break;
		}
		
	}

	FOR_EACH_VEC( removeFlora, idx )
	{
		removeFlora[idx]->Remove();
	}

#ifndef CLIENT_DLL
	if( EditorSystem()->IsActive() )
	{
		KeyValues *pOperation = new KeyValues( "data" );
		pOperation->SetString("operation", "delete");
		pOperation->SetString("classname", "wars_flora");

		// TODO: Specify IDs for flora to be deleted
		/*KeyValues *pEntValues = new KeyValues( "keyvalues" );
		pEntity->FillKeyValues( pEntValues );
		pEntValues->SetString( "model", pModelname );
		pEntValues->SetString( "angles", VarArgs( "%f %f %f", vAngle.x, vAngle.y, vAngle.z ) );
		pEntValues->SetString( "origin", VarArgs( "%f %f %f", vPosition.x, vPosition.y, vPosition.z ) );

		pOperation->AddSubKey( pEntValues );*/

		warseditorstorage->AddEntityToQueue( pOperation );
	}
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CWarsFlora::CountFloraInRadius( const Vector &vPosition, float fRadius )
{
	CUtlVector<int> restrictModels; // Empty for no restriction
	return CountFloraInRadius( vPosition, fRadius, restrictModels );
}

int CWarsFlora::CountFloraInRadius( const Vector &vPosition, float fRadius, CUtlVector<int> &restrictModels )
{
	int iCount = 0;
	float fRadiusSqr = fRadius * fRadius;
#ifdef CLIENT_DLL
	for( CBaseEntity *pEntity = ClientEntityList().FirstBaseEntity(); pEntity; pEntity = ClientEntityList().NextBaseEntity( pEntity ) )
#else
	for( CBaseEntity *pEntity = gEntList.FirstEnt(); pEntity != NULL; pEntity = gEntList.NextEnt( pEntity ) )
#endif // CLIENT_DLL
	{
		CWarsFlora *pFlora = dynamic_cast<CWarsFlora *>( pEntity );
		if( pFlora && pFlora->GetAbsOrigin().DistToSqr( vPosition ) < fRadiusSqr && (!restrictModels.Count() || restrictModels.Find(pFlora->GetModelIndex()) != -1) )
		{
			iCount++;
		}
		
	}

	return iCount;
}

int CWarsFlora::PyCountFloraInRadius( const Vector &vPosition, float fRadius, boost::python::list pyRestrictModels )
{
	CUtlVector<int> restrictModels;
	ListToUtlVectorByValue<int>( pyRestrictModels, restrictModels );
	return CountFloraInRadius( vPosition, fRadius, restrictModels );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class WarsFloraDestructAnimation
{
public:
	bool operator()( CWarsFlora *flora )
	{
		flora->PlayDestructionAnimation();
		return true;
	}
};

void CWarsFlora::DestructFloraInRadius( const Vector &vPosition, float fRadius )
{
	WarsFloraDestructAnimation functor;
	ForAllFloraInRadius<WarsFloraDestructAnimation>( functor, vPosition, fRadius );
}

//-----------------------------------------------------------------------------
// Purpose: Put flora on fire!
//-----------------------------------------------------------------------------
class WarsFloraIgnite
{
public:
	WarsFloraIgnite( float fLifetime ) : m_fLifetime(fLifetime) {}

	bool operator()( CWarsFlora *flora )
	{
		flora->IgniteLifetime( m_fLifetime );
		return true;
	}

private:
	float m_fLifetime;
};
void CWarsFlora::IgniteFloraInRadius( const Vector &vPosition, float fRadius, float fLifetime )
{
#ifdef CLIENT_DLL
	WarsFloraIgnite functor( fLifetime );
	ForAllFloraInRadius<WarsFloraIgnite>( functor, vPosition, fRadius );
#else
	CPVSFilter filter( vPosition );
	g_TEIgniteFloraEvent.m_vecOrigin = vPosition;
	g_TEIgniteFloraEvent.m_fRadius = fRadius;
	g_TEIgniteFloraEvent.m_fLifetime = fLifetime;
	g_TEIgniteFloraEvent.Create( filter, 0.0f );
#endif // CLIENT_DLL
}

#ifdef CLIENT_DLL
CON_COMMAND_F( cl_wars_flora_spawn, "Spawns the specified flora model", FCVAR_CHEAT )
#else
CON_COMMAND_F( wars_flora_spawn, "Spawns the specified flora model", FCVAR_CHEAT )
#endif // CLIENT_DLL
{
	if( args.ArgC() < 2 )
	{
		Warning("wars_flora_spawn: Not enough arguments.\n Example usage: wars_flora_spawn <modelname> <randomradius>");
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

	if( !info.m_bSuccess || info.m_vPosition == vec3_origin )
	{
		Warning("wars_flora_spawn: Could not find a valid position\n");
		return;
	}

	QAngle angles(0, random->RandomFloat(0, 360), 0);
	CWarsFlora::SpawnFlora( pModelName, info.m_vPosition, angles );

#ifndef CLIENT_DLL
	if( !EditorSystem()->IsActive() )
	{
		for( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
			if( !pPlayer || !pPlayer->IsConnected() )
				continue;
			engine->ClientCommand( pPlayer->edict(), VarArgs( "cl_wars_flora_spawn %s", args.ArgS() ) );
		}
	}
#endif // CLIENT_DLL
}

#ifdef CLIENT_DLL
CON_COMMAND_F( cl_wars_flora_grid_mouse, "", FCVAR_CHEAT )
#else
CON_COMMAND_F( wars_flora_grid_mouse, "", FCVAR_CHEAT )
#endif // CLIENT_DLL
{
#ifdef CLIENT_DLL
	CHL2WarsPlayer *pPlayer = CHL2WarsPlayer::GetLocalHL2WarsPlayer(); 
#else
	CHL2WarsPlayer *pPlayer = ToHL2WarsPlayer( UTIL_PlayerByIndex( UTIL_GetCommandClientIndex() ) );
#endif // CLIENT_DLL
	if( !pPlayer )
		return;

	const MouseTraceData_t &data = pPlayer->GetMouseData();

	int iKey = FLORA_KEY( data.m_vWorldOnlyEndPos );

	Msg( "%d Flora at mouse position\n", s_FloraGrid[iKey].Count() );
}

#ifdef CLIENT_DLL
CON_COMMAND_F( cl_wars_flora_ignite, "", FCVAR_CHEAT )
#else
CON_COMMAND_F( wars_flora_ignite, "", FCVAR_CHEAT )
#endif // CLIENT_DLL
{
#ifdef CLIENT_DLL
	CHL2WarsPlayer *pPlayer = CHL2WarsPlayer::GetLocalHL2WarsPlayer(); 
#else
	CHL2WarsPlayer *pPlayer = ToHL2WarsPlayer( UTIL_PlayerByIndex( UTIL_GetCommandClientIndex() ) );
#endif // CLIENT_DLL
	if( !pPlayer )
		return;

	const MouseTraceData_t &data = pPlayer->GetMouseData();

	CWarsFlora::IgniteFloraInRadius( data.m_vWorldOnlyEndPos, args.ArgC() > 1 ? atof( args[1] ) : 128.0f );
}


#ifdef CLIENT_DLL
CON_COMMAND_F( cl_wars_flora_data_reload, "", FCVAR_CHEAT )
#else
CON_COMMAND_F( wars_flora_data_reload, "", FCVAR_CHEAT )
#endif // CLIENT_DLL
{
	CWarsFlora::InitFloraDataKeyValues();
}
