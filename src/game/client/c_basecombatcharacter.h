//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Defines the client-side representation of CBaseCombatCharacter.
//
// $NoKeywords: $
//===========================================================================//

#ifndef C_BASECOMBATCHARACTER_H
#define C_BASECOMBATCHARACTER_H

#ifdef _WIN32
#pragma once
#endif

#include "shareddefs.h"
#include "c_baseflex.h"

#define BCC_DEFAULT_LOOK_TOWARDS_TOLERANCE 0.9f

class C_BaseCombatWeapon;
class C_WeaponCombatShield;
class CNavArea;

// Place somewhere shared?
enum Disposition_t 
{
	D_ER,		// Undefined - error
	D_HT,		// Hate
	D_FR,		// Fear
	D_LI,		// Like
	D_NU,		// Neutral

	// The following are duplicates of the above, only with friendlier names
	D_ERROR = D_ER,		// Undefined - error
	D_HATE = D_HT,		// Hate
	D_FEAR = D_FR,		// Fear
	D_LIKE = D_LI,		// Like
	D_NEUTRAL = D_NU,	// Neutral
};

class C_BaseCombatCharacter : public C_BaseFlex
{
	DECLARE_CLASS( C_BaseCombatCharacter, C_BaseFlex );
public:
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_PYCLIENTCLASS( C_BaseCombatCharacter, PN_BASECOMBATCHARACTER );

					C_BaseCombatCharacter( void );
	virtual			~C_BaseCombatCharacter( void );

	virtual bool	IsBaseCombatCharacter( void ) { return true; };
	virtual C_BaseCombatCharacter *MyCombatCharacterPointer( void ) { return this; }

	// -----------------------
	// Vision
	// -----------------------
	enum FieldOfViewCheckType { USE_FOV, DISREGARD_FOV };
	bool IsAbleToSee( const CBaseEntity *entity, FieldOfViewCheckType checkFOV );	// Visible starts with line of sight, and adds all the extra game checks like fog, smoke, camo...
	bool IsAbleToSee( C_BaseCombatCharacter *pBCC, FieldOfViewCheckType checkFOV );	// Visible starts with line of sight, and adds all the extra game checks like fog, smoke, camo...

	virtual bool IsLookingTowards( const CBaseEntity *target, float cosTolerance = BCC_DEFAULT_LOOK_TOWARDS_TOLERANCE ) const;	// return true if our view direction is pointing at the given target, within the cosine of the angular tolerance. LINE OF SIGHT IS NOT CHECKED.
	virtual bool IsLookingTowards( const Vector &target, float cosTolerance = BCC_DEFAULT_LOOK_TOWARDS_TOLERANCE ) const;	// return true if our view direction is pointing at the given target, within the cosine of the angular tolerance. LINE OF SIGHT IS NOT CHECKED.

	virtual bool IsInFieldOfView( CBaseEntity *entity ) const;	// Calls IsLookingAt with the current field of view.  
	virtual bool IsInFieldOfView( const Vector &pos ) const;

	enum LineOfSightCheckType
	{
		IGNORE_NOTHING,
		IGNORE_ACTORS
	};
	virtual bool IsLineOfSightClear( CBaseEntity *entity, LineOfSightCheckType checkType = IGNORE_NOTHING ) const;// strictly LOS check with no other considerations
	virtual bool IsLineOfSightClear( const Vector &pos, LineOfSightCheckType checkType = IGNORE_NOTHING, CBaseEntity *entityToIgnore = NULL ) const;

	// -----------------------
	// Ammo
	// -----------------------
	void				RemoveAmmo( int iCount, int iAmmoIndex );
	void				RemoveAmmo( int iCount, const char *szName );
	void				RemoveAllAmmo( );
	int					GetAmmoCount( int iAmmoIndex ) const;
	int					GetAmmoCount( char *szName ) const;

	virtual C_BaseCombatWeapon*	Weapon_OwnsThisType( const char *pszWeapon, int iSubType = 0 ) const;  // True if already owns a weapon of this class
	virtual int			Weapon_GetSlot( const char *pszWeapon, int iSubType = 0 ) const;  // Returns -1 if they don't have one
	virtual	bool		Weapon_Switch( C_BaseCombatWeapon *pWeapon, int viewmodelindex = 0 );
	virtual bool		Weapon_CanSwitchTo(C_BaseCombatWeapon *pWeapon);
	
	// I can't use my current weapon anymore. Switch me to the next best weapon.
	bool SwitchToNextBestWeapon(C_BaseCombatWeapon *pCurrent);

	virtual C_BaseCombatWeapon	*GetActiveWeapon( void ) const;
	int							WeaponCount() const;
	virtual C_BaseCombatWeapon	*GetWeapon( int i ) const;

	// This is a sort of hack back-door only used by physgun!
	void SetAmmoCount( int iCount, int iAmmoIndex );

	float				GetNextAttack() const { return m_flNextAttack; }
	void				SetNextAttack( float flWait ) { m_flNextAttack = flWait; }

	virtual int			BloodColor();

	// Blood color (see BLOOD_COLOR_* macros in baseentity.h)
	void SetBloodColor( int nBloodColor );

	virtual void DoMuzzleFlash();

	virtual	Vector		Weapon_ShootPosition( );		// gun position at current position/orientation

	// Weapons
	void				Weapon_SetActivity( Activity newActivity, float duration );
	virtual void		Weapon_FrameUpdate( void );

	// Nav mesh
	virtual CNavArea *GetLastKnownArea( void ) const		{ return m_lastNavArea; }		// return the last nav area the player occupied - NULL if unknown
	virtual bool IsAreaTraversable( const CNavArea *area ) const;							// return true if we can use the given area 
	virtual void ClearLastKnownArea( void );
	virtual void UpdateLastKnownArea( void );										// invoke this to update our last known nav area (since there is no think method chained to CBaseCombatCharacter)
	virtual void OnNavAreaChanged( CNavArea *enteredArea, CNavArea *leftArea ) { }	// invoked (by UpdateLastKnownArea) when we enter a new nav area (or it is reset to NULL)
	virtual void OnNavAreaRemoved( CNavArea *removedArea );


public:

// BEGIN PREDICTION DATA COMPACTION (these fields are together to allow for faster copying in prediction system)
	float			m_flNextAttack;

private:
	bool ComputeLOS( const Vector &vecEyePosition, const Vector &vecTarget ) const;

private:
	CNetworkArray( int, m_iAmmo, MAX_AMMO_TYPES );
	CHandle<C_BaseCombatWeapon>		m_hMyWeapons[MAX_WEAPONS];
	CHandle< C_BaseCombatWeapon > m_hActiveWeapon;

// END PREDICTION DATA COMPACTION

protected:

	int			m_bloodColor;			// color of blood particless

	// last known navigation area of player - NULL if unknown
	CNavArea *m_lastNavArea;
	int m_registeredNavTeam;	// ugly, but needed to clean up player team counts in nav mesh
	

public:
	Vector		m_HackedGunPos;			// HACK until we can query end of gun

private:



private:
	C_BaseCombatCharacter( const C_BaseCombatCharacter & ); // not defined, not accessible


//-----------------------


};

inline C_BaseCombatCharacter *ToBaseCombatCharacter( C_BaseEntity *pEntity )
{
	return pEntity ? pEntity->MyCombatCharacterPointer() : NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline int	C_BaseCombatCharacter::WeaponCount() const
{
	return MAX_WEAPONS;
}

EXTERN_RECV_TABLE(DT_BaseCombatCharacter);

#endif // C_BASECOMBATCHARACTER_H
