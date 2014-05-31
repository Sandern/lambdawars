//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose:		Base Unit
//
//=============================================================================//

#ifndef UNIT_BASE_SHARED_H
#define UNIT_BASE_SHARED_H

#ifdef _WIN32
#pragma once
#endif

#include "iunit.h"
#if defined( CLIENT_DLL )
#include "c_basecombatcharacter.h"
#else
#include "basecombatcharacter.h"
#include "unit_base.h"
#include "ai_speech.h"

extern ConVar g_unit_force_minimal_sendtable;
#endif

#if defined( CLIENT_DLL )
#define CUnitBase C_UnitBase
#define CBaseCombatCharacter C_BaseCombatCharacter
#define CHL2WarsPlayer C_HL2WarsPlayer

class CGlowObject;
#endif

#define SENDPROP_HEALTH_BITS_LOW 5 // Bits used for health when sending the minimal info

class CHL2WarsPlayer;
class CUnitBase;
class UnitBaseLocomotion;
class UnitBaseMoveCommand;
class UnitBaseSense;
class UnitBaseNavigator;
class UnitExpresser;
class UnitBaseAnimState;

//=============================================================================
//
// Unit lists, sorted on ownernumber
//
//=============================================================================
struct UnitListInfo
{
	int m_OwnerNumber;
	CUnitBase *m_pHead;
	UnitListInfo *m_pNext;
};

extern UnitListInfo *g_pUnitListHead;

UnitListInfo *GetUnitListForOwnernumber(int iOwnerNumber);
#ifdef ENABLE_PYTHON
	void MapUnits( boost::python::object method );
#endif // ENABLE_PYTHON

//=============================================================================
//
// class CUnit_Manager
//
// Central location for components of the Units to operate across all Units without
// iterating over the global list of entities.
//
//=============================================================================

class CUnit_Manager
{
public:
	CUnit_Manager();
	
	CUnitBase **	AccessUnits();
	int				NumUnits();
	
	void AddUnit( CUnitBase *pUnit );
	void RemoveUnit( CUnitBase *pUnit );

	bool FindUnit( CUnitBase *pUnit )	{ return ( m_Units.Find( pUnit ) != m_Units.InvalidIndex() ); }
	
private:
	enum
	{
		MAX_UNITS = 1024
	};
	
	typedef CUtlVector<CUnitBase *> CUnitArray;
	
	CUnitArray m_Units;

};

//-------------------------------------

extern CUnit_Manager g_Unit_Manager;

//-----------------------------------------------------------------------------
extern Disposition_t g_playerrelationships[MAX_PLAYERS][MAX_PLAYERS];
void SetPlayerRelationShip(int p1, int p2, Disposition_t rel);
Disposition_t GetPlayerRelationShip(int p1, int p2);

#ifdef CLIENT_DLL
	const int DEF_RELATIONSHIP_PRIORITY = INT_MIN;
#endif // CLIENT_DLL

struct UnitRelationship_t
{
	EHANDLE			entity;			// Relationship to a particular entity
	Disposition_t	disposition;	// D_HT (Hate), D_FR (Fear), D_LI (Like), D_NT (Neutral)
	int				priority;		// Relative importance of this relationship (higher numbers mean more important)
};


//=============================================================================
//
//	class CUnitBase
//
//=============================================================================
#ifdef CLIENT_DLL
class CUnitBase : public CBaseCombatCharacter, public IUnit
#else
class CUnitBase : public CBaseCombatCharacter, public CAI_ExpresserSink, public IUnit
#endif // CLIENT_DLL
{
	DECLARE_CLASS( CUnitBase, CBaseCombatCharacter );
public:
	friend class UnitBaseLocomotion;
	friend class UnitBaseNavigator;
	friend class UnitBaseSense;

	//-----------------------------------------------------
	//
	// Initialization, cleanup
	//

	CUnitBase();
	~CUnitBase();

	virtual void		UpdateOnRemove( void );

	//---------------------------------
#if !defined( CLIENT_DLL )
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();
#else
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
#endif

#ifdef CLIENT_DLL
	DECLARE_PYCLIENTCLASS( C_UnitBase, PN_UNITBASE );
#else
	DECLARE_PYSERVERCLASS( CUnitBase, PN_UNITBASE );
#endif // CLIENT_DLL

	virtual IMouse *GetIMouse()												{ return this; }
	virtual IUnit *GetIUnit()												{ return this; }
	virtual CUnitBase *MyUnitPointer( void )								{ return this; }

	virtual bool IsUnit() { return true; }

	CUnitBase *GetNext() { return m_pNext; }
	virtual void OnChangeOwnerNumberInternal( int old_owner_number );

	virtual void Spawn( void );
	virtual void OnRestore( void );

#ifndef CLIENT_DLL
	virtual bool KeyValue( const char *szKeyName, const char *szValue );

	Class_T Classify ( void );
	
	virtual int ShouldTransmit( const CCheckTransmitInfo *pInfo );
	virtual int	UpdateTransmitState();

	bool UseMinimalSendTable( int iClientIndex ); // Only used by proxies
	void SetUseMinimalSendTable( int iClientIndex, bool state );
	void SetAlwaysSendFullSelectionData( bool state ) { m_bAlwaysSendFullSelectionData = state; } // For buildings, see m_bAlwaysSendFullSelectionData
	bool AlwaysSendFullSelectionData() { return m_bAlwaysSendFullSelectionData; }

#endif // CLIENT_DLL

	// IMouse implementation
	virtual void OnClickLeftPressed( CHL2WarsPlayer *player )				{}
	virtual void OnClickRightPressed( CHL2WarsPlayer *player )				{}
	virtual void OnClickLeftReleased( CHL2WarsPlayer *player )				{}
	virtual void OnClickRightReleased( CHL2WarsPlayer *player )				{}

	virtual void OnClickLeftDoublePressed( CHL2WarsPlayer *player )			{}
	virtual void OnClickRightDoublePressed( CHL2WarsPlayer *player )		{}

	virtual void OnCursorEntered( CHL2WarsPlayer *player )					{}
	virtual void OnCursorExited( CHL2WarsPlayer *player )					{}

#ifdef CLIENT_DLL
	virtual void OnHoverPaint();
	virtual unsigned long GetCursor()										{ return 2; }

	virtual int				DrawModel( int flags, const RenderableInstance_t &instance );
	//virtual bool			SetupBones( matrix3x4a_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime );
	void					Blink( float blink_time = 3.0f );

	virtual const Vector&			GetRenderOrigin( void );
	virtual const QAngle&			GetRenderAngles();

	virtual bool		OnInternalDrawModel( ClientModelRenderInfo_t *pInfo );
	void				EnableTeamColorGlow( );
	void				DisableTeamColorGlow( );

#endif // CLIENT_DLL
	virtual void		DoMuzzleFlash();

	// IUnit implementation
	virtual bool		AreAttacksPassable( CBaseEntity *pTarget );

	const char *		GetUnitType();
#ifndef CLIENT_DLL
	void				SetUnitType( const char *unit_type );
#else
	virtual void		OnDataChanged( DataUpdateType_t updateType );

#ifdef CLIENT_DLL
	virtual void		SetHealth(int iHealth) { m_iHealth = iHealth; }
#endif // CLIENT_DLL
	virtual int			GetHealth() const { return m_iHealth; }
	virtual int			GetMaxHealth()  const	{ return m_iMaxHealth; }

	virtual bool		ShouldDraw( void );
	virtual void		UpdateClientSideAnimation();

	virtual void		OnActiveWeaponChanged() {}

	virtual void		InitPredictable( C_BasePlayer *pOwner );
	virtual void		PostDataUpdate( DataUpdateType_t updateType );
	virtual bool		ShouldPredict( void );
	virtual C_BasePlayer* GetPredictionOwner();

	void				EstimateAbsVelocity( Vector& vel );
	//virtual float		GetInterpolationAmount( int flags );

	// prediction smoothing on elevators
	void NotePredictionError( const Vector &vDelta );
	void GetPredictionErrorSmoothingVector( Vector &vOffset );
	Vector m_vecPredictionError;
	float m_flPredictionErrorTime;

#endif 
	virtual void		OnUnitTypeChanged( const char *old_unit_type );

	// Relationships
	Disposition_t		IRelationType( CBaseEntity *pTarget );
	int					IRelationPriority( CBaseEntity *pTarget );

	virtual void		AddEntityRelationship( CBaseEntity *pEntity, Disposition_t nDisposition, int nPriority );
	virtual bool		RemoveEntityRelationship( CBaseEntity *pEntity );
	bool				HasOverridenEntityRelationship( CBaseEntity *pEntity );

	virtual void		MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType );
	virtual const char	*GetTracerType( void );
	virtual void		DoImpactEffect( trace_t &tr, int nDamageType );
	virtual void		TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );

	// Energy
	int					GetEnergy() const { return m_iEnergy; }
	int					GetMaxEnergy()  const	{ return m_iMaxEnergy; }
	void				SetEnergy( int iEnergy ) { m_iEnergy = iEnergy; }
	void				SetMaxEnergy( int iMaxEnergy ) { m_iMaxEnergy = iMaxEnergy; }

	// Kills
	int					GetKills() const { return m_iKills; }
	void				SetKills( int iKills ) { m_iKills = iKills; }

public:
	virtual void		FireBullets( const FireBulletsInfo_t &info );
	virtual bool		TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr );

	// UNDONE: Make this data?
	virtual unsigned int	PhysicsSolidMaskForEntity( void ) const;

	// Special code for moving to target buildings
	virtual bool				HasEnterOffset( void );
	virtual const Vector &		GetEnterOffset( void );
	virtual void				SetEnterOffset( const Vector &enteroffset );

	// Anim state
#ifdef ENABLE_PYTHON
	boost::python::object PyGetAnimState();
	void				SetAnimState( boost::python::object animstate );
#endif // ENABLE_PYTHON
	UnitBaseAnimState*	GetAnimState();

	// Note: on the client, this is based on the health variable decreasing
	float				GetLastTakeDamageTime() { return m_fLastTakeDamageTime; }

#ifndef CLIENT_DLL
	virtual void		UpdateServerAnimation( void );
	
	virtual int			OnTakeDamage( const CTakeDamageInfo &info );
	virtual int			OnTakeDamage_Alive( const CTakeDamageInfo &info );
	int					TakeHealth( float flHealth, int bitsDamageType );
	virtual void		OnFullHealth( void ) {}
	virtual void		OnLostFullHealth( void ) {}

	void				DispatchOutOfAmmo( void );
	void				DispatchBurstFinished( void );

	virtual float		GetDensityMultiplier();

	// Weapons
	virtual void		Weapon_Equip( CBaseCombatWeapon *pWeapon );	

	// Enemy management
	void				SetEnemy( CBaseEntity *pEnt );
	void				UpdateEnemy( UnitBaseSense &senses );
	void				CheckEnemyAlive( void );
	virtual bool		PassesDamageFilter( const CTakeDamageInfo &info );

	// Enemy/attack related
	virtual bool		HasRangeAttackLOS( const Vector &vTargetPos, CBaseEntity *pTarget = NULL );
	virtual bool		HasRangeAttackLOSTarget( CBaseEntity *pTarget );
	float				EnemyDistance( CBaseEntity *pEnemy, bool bConsiderSizeUnit=true );
	virtual float		TargetDistance( const Vector &pos, CBaseEntity *pTarget, bool bConsiderSizeUnit=true );
	virtual bool		FInAimCone( CBaseEntity *pEntity, float fMinDot=0.994f );
	virtual bool		FInAimCone( const Vector &vecSpot, float fMinDot=0.994f );
	void				SetAttackLOSMask( int iMask ) { m_iAttackLOSMask = iMask; }
	int					GetAttackLOSMask() { return m_iAttackLOSMask; }
	bool				FastLOSCheck( const Vector &vTargetPos );

	// Navigator
#ifdef ENABLE_PYTHON
	boost::python::object PyGetNavigator();
	void				SetNavigator( boost::python::object navigator );
#endif // ENABLE_PYTHON
	UnitBaseNavigator*	GetNavigator();

	// Expresser
	IResponseSystem *	GetResponseSystem();

	// Anim Event map
#ifdef ENABLE_PYTHON
	void SetAnimEventMap( boost::python::object animeventmap );
#endif // ENABLE_PYTHON
	virtual void		HandleAnimEvent( animevent_t *pEvent );

#endif

	CBaseEntity *		GetEnemy();

	virtual Vector		GetShootEnemyDir( Vector &shootOrigin, bool noisy = true );
	virtual Vector		BodyTarget( const Vector &posSrc, bool bNoisy = true );

	void				SetCanBeSeen( bool canbeseen ) { m_bCanBeSeen = canbeseen; }
	bool				CanBeSeen( CUnitBase *pUnit = NULL ) { if( UseCustomCanBeSeenCheck() ) return CustomCanBeSeen( pUnit ); return m_bCanBeSeen; }
	virtual bool		CustomCanBeSeen( CUnitBase *pUnit = NULL ) { return true; }
	bool				UseCustomCanBeSeenCheck() { return m_bUseCustomCanBeSeenCheck; }
	void				SetUseCustomCanBeSeenCheck( bool bUseCustomCanBeSeen ) { m_bUseCustomCanBeSeenCheck = bUseCustomCanBeSeen; }

	void				SetDefaultEyeOffset ( Vector *pCustomOfset = NULL );
	const Vector &		GetDefaultEyeOffset( void )			{ return m_vDefaultEyeOffset;	}

#ifdef ENABLE_PYTHON
	virtual bool		IsSelectableByPlayer( CHL2WarsPlayer *pPlayer, boost::python::object target_selection = boost::python::object() ) { return true; }
#endif // ENABLE_PYTHON
	virtual void		Select( CHL2WarsPlayer *pPlayer, bool bTriggerOnSel = true );
	virtual void		OnSelected( CHL2WarsPlayer *pPlayer );
	virtual void		OnDeSelected( CHL2WarsPlayer *pPlayer );
	bool				IsSelectedByPlayer( int entindex );
	const CUtlVector< CHandle< CHL2WarsPlayer > > &GetSelectedByPlayers();
#ifdef CLIENT_DLL
	virtual void OnInSelectionBox( void ) {}
	virtual void OnOutSelectionBox( void ) {}
#endif // CLIENT_DLL
	virtual int GetSelectionPriority();
	virtual void SetSelectionPriority( int priority );
	virtual int GetAttackPriority();
	virtual void SetAttackPriority( int priority );

	// Action
	virtual void Order( CHL2WarsPlayer *pPlayer )										{}

	// Squads
	virtual  CBaseEntity *GetSquad();
#ifndef CLIENT_DLL
	virtual void SetSquad( CBaseEntity *pUnit ) { m_hSquadUnit = pUnit; }
#endif // CLIENT_DLL

	// Player control
	virtual void UserCmd( CUserCmd *pCmd )												{}
	virtual void OnUserControl( CHL2WarsPlayer *pPlayer );
	virtual void OnUserLeftControl( CHL2WarsPlayer *pPlayer )							{}
	virtual bool CanUserControl( CHL2WarsPlayer *pPlayer )								{ return true; }
	virtual void OnButtonsChanged( int buttonsMask, int buttonsChanged )				{}
#ifdef CLIENT_DLL
	virtual bool SelectSlot( int slot )													{ return false; }
#else
	virtual bool ClientCommand( const CCommand &args )									{ return false; }
#endif // CLIENT_DLL

#ifndef CLIENT_DLL
	void SetCommander(CHL2WarsPlayer *player);		// sets which player commands this unit
#endif // CLIENT_DLL
	CHL2WarsPlayer* GetCommander() const;

	virtual void PhysicsSimulate( void );

	// Animation state
#ifndef CLIENT_DLL
	void SetCrouching( bool crouching );
	void SetClimbing( bool climbing );
#endif // CLIENT_DLL
	bool IsCrouching( void );
	bool IsClimbing( void );

private:
	void AddToUnitList();
	void RemoveFromUnitList();

public:
	// FOW Variables
	bool m_bFOWFilterFriendly;

	float m_fEyeYaw;
#ifndef CLIENT_DLL
	CNetworkVar(float, m_fEyePitch );
#else
	float m_fEyePitch;
#endif // CLIENT_DLL

	bool m_bNeverIgnoreAttacks;
	bool m_bBodyTargetOriginBased;
	bool m_bFriendlyDamage;

	float m_fAccuracy;

	// Parameters for the navigator/pathfinding/navmesh
	float m_fSaveDrop;
	float m_fDeathDrop;
	float m_fMaxClimbHeight;
	float m_fTestRouteStartHeight;
	float m_fMinSlope;

#ifndef CLIENT_DLL
	// SERVER VARIABLES
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_iHealth );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_iMaxHealth );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_fFlags );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_lifeState );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_takedamage );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_hGroundEntity );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_vecBaseVelocity );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_vecVelocity );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_vecViewOffset );

	// Enemy related
	float m_fEnemyChangeToleranceSqr;
#else
	// CLIENT VARIABLES
	int m_iMaxHealth;

	// Animation
	bool m_bUpdateClientAnimations;
#endif // CLIENT_DLL

protected:
	Vector m_vDefaultEyeOffset;

private:
	// Unit list
	UnitListInfo *m_pUnitList;
	CUnitBase *m_pPrev;
	CUnitBase *m_pNext;

	bool m_bCanBeSeen;
	bool m_bUseCustomCanBeSeenCheck;
	int m_iSelectionPriority;
	int m_iAttackPriority;

	// Players that have this unit selected
	CUtlVector< CHandle< CHL2WarsPlayer > > m_SelectedByPlayers;

	// Entity relationships
	CUtlVector<UnitRelationship_t>		m_Relationship;

	bool m_bHasEnterOffset;
	Vector m_vEnterOffset; 

	// Animstate
	UnitBaseAnimState *m_pAnimState;
#ifdef ENABLE_PYTHON
	boost::python::object m_pyAnimState;
#endif // ENABLE_PYTHON

	// Last damage time
	float m_fLastTakeDamageTime;

#ifndef CLIENT_DLL
	string_t						m_UnitType;
	CNetworkVar(int, m_NetworkedUnitTypeSymbol );

	CPlayerBitVec m_UseMinimalSendTable;
	bool m_bAlwaysSendFullSelectionData; // Always send health/energy full resolution (for buildings)

	bool m_bHasEnemy;
	float m_fLastEnemyChangeTime;

	bool m_bHasRangeAttackLOS;
	float m_fLastRangeAttackLOSTime;
	int m_iAttackLOSMask;

	// Navigator
	UnitBaseNavigator *m_pNavigator;
#ifdef ENABLE_PYTHON
	boost::python::object m_pyNavigator;
#endif // ENABLE_PYTHON

	// Expresser
	UnitExpresser *m_pExpresser;
#ifdef ENABLE_PYTHON
	boost::python::object m_pyExpresser;
#endif // ENABLE_PYTHON

	// Anim Event handler map
	AnimEventMap *m_pAnimEventMap;
#ifdef ENABLE_PYTHON
	boost::python::object m_pyAnimEventMap; // Keeps m_pAnimEventMap valid
#endif // ENABLE_PYTHON

	float m_fNextServerAnimStateTime;

	// Animation state
	CNetworkVar(bool, m_bCrouching );
	CNetworkVar(bool, m_bClimbing );

	// Energy
	CNetworkVar(int, m_iEnergy );
	CNetworkVar(int, m_iMaxEnergy );

	// Kills
	CNetworkVar(int, m_iKills );
#else
	string_t m_UnitType;
	int m_NetworkedUnitTypeSymbol;
	int m_OldNetworkedUnitTypeSymbol;

	CHandle< CHL2WarsPlayer > m_hOldCommander;
	CHandle< C_BaseCombatWeapon > m_hOldActiveWeapon;
	int						m_iOldHealth;

	CHandle< CBaseEntity >	m_hOldEnemy;
	bool					m_bForcedEnemyHate;

	// Target unit/produced new unit: blink
	bool					m_bIsBlinking;
	float					m_fBlinkTimeOut;

	// Animation state
	bool					m_bCrouching;
	bool					m_bClimbing;

	// Energy
	int						m_iEnergy;
	int						m_iMaxEnergy;

	// Kills
	int						m_iKills;

	// Client simulation time
	float					m_fNextSimulationUpdate;
	Vector					m_vecOldUnitOrigin;

	// Glow
	CGlowObject *m_pTeamColorGlowEffect;
#endif // CLIENT_DLL

	CNetworkHandle( CBaseEntity, m_hSquadUnit );
	CNetworkHandle (CHL2WarsPlayer, m_hCommander); 	// the player in charge of this unit
	CNetworkHandle( CBaseEntity, m_hEnemy );
};

typedef CHandle<CUnitBase> UNITHANDLE;

// Inlines
inline CBaseEntity *CUnitBase::GetSquad() 
{ 
	return m_hSquadUnit.Get(); 
}

inline CBaseEntity *CUnitBase::GetEnemy()
{
	return m_hEnemy.Get();
}

inline bool CUnitBase::IsCrouching( void )
{
	return m_bCrouching;
}

inline bool CUnitBase::IsClimbing( void )
{
	return m_bClimbing;
}

inline int CUnitBase::GetSelectionPriority()
{
	return m_iSelectionPriority;
}

inline void CUnitBase::SetSelectionPriority( int priority )
{
	m_iSelectionPriority = priority;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline bool CUnitBase::IsSelectedByPlayer( int entindex )
{
	for( int i = 0; i < m_SelectedByPlayers.Count(); i++ )
	{
		if( m_SelectedByPlayers[i].IsValid() && m_SelectedByPlayers[i].GetEntryIndex() == entindex )
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline const CUtlVector< CHandle< CHL2WarsPlayer > > &CUnitBase::GetSelectedByPlayers()
{
	return m_SelectedByPlayers;
}

inline int CUnitBase::GetAttackPriority()
{
	return m_iAttackPriority;
}

inline void CUnitBase::SetAttackPriority( int priority )
{
	m_iAttackPriority = priority;
}

//-----------------------------------------------------------------------------
// Purpose: Tests if the unit has a specific added relationship for the target entity
//-----------------------------------------------------------------------------
inline bool CUnitBase::HasOverridenEntityRelationship( CBaseEntity *pEntity )
{
	for ( int i = 0; i < m_Relationship.Count(); i++ )
	{
		if( m_Relationship[i].entity == pEntity )
			return true;
	}
	return false;
}

#ifdef ENABLE_PYTHON
inline boost::python::object CUnitBase::PyGetAnimState() 
{
	return m_pyAnimState;
}
#endif // ENABLE_PYTHON

inline UnitBaseAnimState *CUnitBase::GetAnimState() 
{ 
	return m_pAnimState; 
}

#ifndef CLIENT_DLL
inline Class_T CUnitBase::Classify ( void )
{
	return CLASS_PLAYER;
}

//-----------------------------------------------------------------------------
// Purpose: Return dist. to enemy (closest of origin/head/feet)
//-----------------------------------------------------------------------------
inline float CUnitBase::EnemyDistance( CBaseEntity *pEnemy, bool bConsiderSizeUnit )
{
	return TargetDistance( WorldSpaceCenter(), pEnemy, bConsiderSizeUnit );
}

#ifdef ENABLE_PYTHON
inline boost::python::object CUnitBase::PyGetNavigator() 
{
	return m_pyNavigator;
}
#endif // ENABLE_PYTHON

inline UnitBaseNavigator *CUnitBase::GetNavigator() 
{ 
	return m_pNavigator; 
}

inline bool CUnitBase::FastLOSCheck( const Vector &vTargetPos )
{
	CTraceFilterWorldOnly filter;
	trace_t tr;
	UTIL_TraceLine( EyePosition(), vTargetPos, m_iAttackLOSMask, &filter, &tr);
	return !tr.DidHit();
}

inline bool CUnitBase::UseMinimalSendTable( int iClientIndex )
{
	return m_UseMinimalSendTable.IsBitSet( iClientIndex );
}
#endif // CLIENT_DLL

inline bool CUnitBase::HasEnterOffset( void ) 
{ 
	return m_bHasEnterOffset; 
}

inline const Vector &CUnitBase::GetEnterOffset( void ) 
{ 
	return m_vEnterOffset; 
}

inline void CUnitBase::SetEnterOffset( const Vector &enteroffset )
{ 
	m_bHasEnterOffset = true;
	m_vEnterOffset = enteroffset; 
}

#endif // UNIT_BASE_SHARED_H