//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "unit_animstate.h"
#include "tier0/vprof.h"
#include "animation.h"
#include "studio.h"
#include "apparent_velocity_helper.h"
#include "utldict.h"
#include "filesystem.h"
#include "gamestringpool.h"

#ifdef CLIENT_DLL
	#include "engine/ivdebugoverlay.h"

	extern ConVar cl_showanimstate;
	extern ConVar showanimstate_log;
	extern ConVar showanimstate_activities;
#else
	extern ConVar sv_showanimstate;
	extern ConVar showanimstate_log;
	extern ConVar showanimstate_activities;
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


// Below this many degrees, slow down turning rate linearly
#define FADE_TURN_DEGREES	45.0f

// After this, need to start turning feet
#define MAX_TORSO_ANGLE		70.0f

// Below this amount, don't play a turning animation/perform IK
#define MIN_TURN_ANGLE_REQUIRING_TURN_ANIMATION		15.0f

ConVar unit_disableanimstate( "unit_disableanimstate", "0", FCVAR_REPLICATED|FCVAR_CHEAT, "Disables updating the animation state of all units." );
ConVar wars_feetyawrate("wars_feetyawrate", "800",  FCVAR_REPLICATED, "How many degrees per second that we can turn our feet or upper body." );
ConVar wars_facefronttime( "wars_facefronttime", "0.0", FCVAR_REPLICATED, "How many seconds before unit faces front when standing still." );

extern ConVar mp_ik;

// Pose parameters stored for debugging.
extern float g_flLastBodyPitch, g_flLastBodyYaw, m_flLastMoveYaw;

// ------------------------------------------------------------------------------------------------ //
// Translate activity map
// ------------------------------------------------------------------------------------------------ //
TranslateActivityMap::TranslateActivityMap( TranslateActivityMap &activitymap )
{
	SetDefLessFunc( m_translateActivityMap );

	FOR_EACH_MAP_FAST(activitymap.m_translateActivityMap, idx)
	{
		AddTranslation(activitymap.m_translateActivityMap.Key(idx), activitymap.m_translateActivityMap.Element(idx));
	}
}

#ifdef ENABLE_PYTHON
TranslateActivityMap::TranslateActivityMap( boost::python::dict d)
{
	SetDefLessFunc( m_translateActivityMap );

	AddTranslations(d);
}

TranslateActivityMap::TranslateActivityMap( TranslateActivityMap &activitymap, boost::python::dict d )
{
	SetDefLessFunc( m_translateActivityMap );

	FOR_EACH_MAP_FAST(activitymap.m_translateActivityMap, idx)
	{
		AddTranslation(activitymap.m_translateActivityMap.Key(idx), activitymap.m_translateActivityMap.Element(idx));
	}

	AddTranslations(d);
}

void TranslateActivityMap::AddTranslations( boost::python::dict d )
{
	Activity a, atrans;
	boost::python::object objectKey, objectValue;
	const boost::python::object objectKeys = d.iterkeys();
	const boost::python::object objectValues = d.itervalues();
	unsigned long ulCount = boost::python::len(d);
	for( unsigned long u = 0; u < ulCount; u++ )
	{
		objectKey = objectKeys.attr( "next" )();
		objectValue = objectValues.attr( "next" )();

		a = boost::python::extract<Activity>(objectKey);
		atrans = boost::python::extract<Activity>(objectValue);

		AddTranslation(a, atrans);
	}
}
#endif // ENABLE_PYTHON

void TranslateActivityMap::DebugPrint()
{
	Msg("Printing Translation Activity Map:\n");
	FOR_EACH_MAP_FAST(m_translateActivityMap, idx)
	{
		Msg("\tidx: %d, key: %d, translation: %d\n", idx, m_translateActivityMap.Key(idx), m_translateActivityMap.Element(idx));
	}
}

// ------------------------------------------------------------------------------------------------ //
// UnitAnimState implementation.
// ------------------------------------------------------------------------------------------------ //
#ifdef ENABLE_PYTHON
UnitAnimState::UnitAnimState(boost::python::object outer, UnitAnimConfig &animconfig) : UnitBaseAnimState(outer)
{
	m_flEyeYaw = 0.0f;
	m_flEyePitch = 0.0f;
	m_bCurrentFeetYawInitialized = false;
	m_flCurrentTorsoYaw = 0.0f;
	m_flCurrentTorsoYaw = TURN_NONE;
	m_flMaxGroundSpeed = 0.0f;
	m_flStoredCycle = 0.0f;

	m_bPlayingMisc = false;
	m_flMiscCycle = 0.0f; 
	m_iMiscSequence = -1; 
	m_flMiscBlendIn = m_flMiscBlendOut = 0.0f;
	m_bMiscOnlyWhenStill = false;
	m_fMiscPlaybackRate = 1.0f;

	m_flGaitYaw = 0.0f;
	m_flGoalFeetYaw = 0.0f;
	m_flCurrentFeetYaw = 0.0f;
	m_flLastYaw = 0.0f;
	m_flLastTurnTime = 0.0f;
	m_angRender.Init();
	m_vLastMovePose.Init();
	m_iCurrent8WayIdleSequence = -1;
	m_iCurrent8WayCrouchIdleSequence = -1;

	m_eCurrentMainSequenceActivity = ACT_IDLE;
	m_nSpecificMainActivity = ACT_INVALID;
	m_bSpecMainUseCustomPlaybackrate = false;
	m_flLastAnimationStateClearTime = 0.0f;

	m_bPlayFallActInAir = true;

	m_iMoveX = -1;
	m_iMoveY = -1;
	m_iMoveYaw = -1;
	m_iBodyYaw = -1;
	m_iBodyPitch = -1;
	m_iLeanYaw = -1;
	m_iLeanPitch = -1;

	m_bFlipMoveY = false;
	m_bNewJump = true;

	m_flMiscBlendOut = 0.1f;
	m_flMiscBlendIn = 0.1f;
	m_fMiscPlaybackRate = 1.0f;

	m_flFeetYawRate = wars_feetyawrate.GetFloat();
	m_flFaceFrontTime = wars_facefronttime.GetFloat();

	m_bUseCombatState = false;
	m_fCombatStateTime = 0.0f;
	m_bCombatStateIfEnemy = false;

	m_AnimConfig = animconfig;
	m_pActivityMap = NULL;
	m_sAimLayerSequence = MAKE_STRING("gesture_shoot_smg1");
	ClearAnimationState();
}
#endif // ENABLE_PYTHON

UnitAnimState::~UnitAnimState()
{
}

void UnitAnimState::ClearAnimationState()
{
	ClearAnimationLayers();
	m_bCurrentFeetYawInitialized = false;
	m_flLastAnimationStateClearTime = gpGlobals->curtime;
	m_bJumping = false;
}


float UnitAnimState::TimeSinceLastAnimationStateClear() const
{
	return gpGlobals->curtime - m_flLastAnimationStateClearTime;
}

void UnitAnimState::Update( float eyeYaw, float eyePitch )
{
	VPROF_BUDGET( "UnitAnimState::Update", VPROF_BUDGETGROUP_UNITS );

	if( unit_disableanimstate.GetBool() )
		return;

	// Clear animation overlays because we're about to completely reconstruct them.
	ClearAnimationLayers();

	// Some mods don't want to update the player's animation state if they're dead and ragdolled.
	if ( !ShouldUpdateAnimState() )
	{
		ClearAnimationState();
		return;
	}

	CStudioHdr *pStudioHdr = GetOuter()->GetModelPtr();
	// Store these. All the calculations are based on them.
	m_flEyeYaw = AngleNormalize( eyeYaw );
	m_flEyePitch = AngleNormalize( eyePitch );


	// Compute sequences for all the layers.
	ComputeSequences( pStudioHdr );
	

	// Compute all the pose params.
	ComputePoseParam_BodyPitch( pStudioHdr );	// Look up/down.
	ComputePoseParam_BodyYaw();		// Torso rotation.
	ComputePoseParam_MoveYaw( pStudioHdr );		// What direction his legs are running in.
	ComputePoseParam_Lean();
	
	ComputePlaybackRate();

#ifdef CLIENT_DLL
	if ( cl_showanimstate.GetInt() == GetOuter()->entindex() )
	{
		DebugShowAnimStateFull( 5 );
	}
#else
	if ( sv_showanimstate.GetInt() == GetOuter()->entindex() )
	{
		DebugShowAnimState( 20 );
	}
#endif

#ifndef CLIENT_DLL
	// Advance the server frame
	// NOTE: Do this last, because it will change the anim time interval
	// NOTE2: StudioFrameAdvance also updates the layers, StudioFrameAdvanceManual does not.
	//StudioFrameAdvance();
	GetOuter()->StudioFrameAdvanceManual( GetAnimTimeInterval() );
	GetOuter()->DispatchAnimEvents(GetOuter());
#endif // CLIENT_DLL
}


bool UnitAnimState::ShouldUpdateAnimState()
{
	// Don't update anim state if we're not visible
	if ( GetOuter()->IsEffectActive( EF_NODRAW ) )
		return false;

	// By default, don't update their animation state when they're dead because they're
	// either a ragdoll or they're not drawn.
#ifdef CLIENT_DLL
	if ( GetOuter()->IsDormant() )
		return false;
#endif

	return GetOuter()->IsAlive();
}

int UnitAnimState::CalcAimLayerSequence( float *flCycle, float *flAimSequenceWeight, bool bForceIdle )
{
	return GetOuter()->LookupSequence( STRING(m_sAimLayerSequence) );
}

// does misc gestures if we're not firing
void UnitAnimState::ComputeMiscSequence()
{
	// Run melee anims on the server so we can get bone positions for damage traces, etc
	//CASW_Marine* pMarine = dynamic_cast<CASW_Marine*>(GetOuter());
	
// 	bool bFiring = pMarine && pMarine->GetActiveASWWeapon() && pMarine->GetActiveASWWeapon()->IsFiring() && !pMarine->GetActiveASWWeapon()->ASWIsMeleeWeapon();
// 	if (m_bPlayingMisc && bFiring)
// 	{
// 		m_bPlayingMisc = false;
// 	}

	//CASW_Melee_Attack *pAttack = pMarine ? pMarine->GetCurrentMeleeAttack() : NULL;
	//bool bHoldAtEnd = pAttack ? pAttack->m_bHoldAtEnd : false;
	bool bHoldAtEnd = false;

	// If this is hit it means MiscCycleRewind() was called, and then ComputeMiscSequence() was called before MiscCycleUnrewind()
	//  Add a call to MiscCycleUnrewind() so that the cycle update in UpdateLayerSequenceGeneric() is correct 
	//Assert( !m_bMiscCycleRewound );

	UpdateLayerSequenceGeneric( RELOADSEQUENCE_LAYER, m_bPlayingMisc, m_flMiscCycle, m_iMiscSequence, bHoldAtEnd, m_flMiscBlendIn, m_flMiscBlendOut, m_bMiscOnlyWhenStill, m_fMiscPlaybackRate );	
}

bool UnitAnimState::ShouldResetGroundSpeed( Activity oldActivity, Activity idealActivity )
{
	// If we went from idle to walk, reset the interpolation history.
	return ( (oldActivity == ACT_CROUCHIDLE || oldActivity == ACT_IDLE || oldActivity == ACT_IDLE_RELAXED
		|| oldActivity == ACT_MP_CROUCHWALK_ITEM1 || oldActivity == ACT_MP_STAND_ITEM1) && 
		(idealActivity == ACT_WALK || idealActivity == ACT_RUN_CROUCH || idealActivity == ACT_WALK_AIM_RIFLE
		|| idealActivity == ACT_WALK_AIM_PISTOL || idealActivity == ACT_RUN || idealActivity == ACT_RUN_AIM_RIFLE
		|| idealActivity == ACT_RUN_AIM_PISTOL || idealActivity == ACT_RUN_RELAXED || idealActivity == ACT_WALK_RELAXED
		|| idealActivity == ACT_MP_RUN_ITEM1 || idealActivity == ACT_MP_WALK_ITEM1) );
}

bool UnitAnimState::ShouldChangeSequences( void ) const
{
	return true;
}





void UnitAnimState::ClearAnimationLayers()
{
	VPROF( "UnitAnimState::ClearAnimationLayers" );
	if ( !GetOuter() )
		return;

	GetOuter()->SetNumAnimOverlays( NUM_LAYERS_WANTED );
	for ( int i=0; i < GetOuter()->GetNumAnimOverlays(); i++ )
	{
		GetOuter()->GetAnimOverlay( i )->SetOrder( CBaseAnimatingOverlay::MAX_OVERLAYS );
#ifndef CLIENT_DLL
		GetOuter()->GetAnimOverlay( i )->m_fFlags = 0;
#endif
	}
}


void UnitAnimState::RestartMainSequence()
{
	CBaseAnimatingOverlay *pUnit = GetOuter();

	pUnit->m_flAnimTime = gpGlobals->curtime;
	pUnit->SetCycle( 0 );

	// Directly activate the specific activity if any
	// Then AutoMovement can be directly executed afterwards, so we don't miss any part.
	if ( m_nSpecificMainActivity != ACT_INVALID )
	{
		if ( pUnit->GetSequenceActivity( pUnit->GetSequence() ) != m_nSpecificMainActivity )
		{
			pUnit->ResetSequence( SelectWeightedSequence( m_nSpecificMainActivity ) );
			return;
		}
	}
}


void UnitAnimState::ComputeSequences( CStudioHdr *pStudioHdr )
{
	VPROF( "UnitAnimState::ComputeSequences" );

	ComputeMainSequence();		// Lower body (walk/run/idle).
	UpdateInterpolators();		// The groundspeed interpolator uses the main sequence info.

	if ( m_AnimConfig.m_bUseAimSequences )
	{
		ComputeAimSequence();		// Upper body, based on weapon type.
	}
	ComputeMiscSequence();
}

void UnitAnimState::ResetGroundSpeed( void )
{
	m_flMaxGroundSpeed = GetCurrentMaxGroundSpeed();
}

bool UnitAnimState::HandleClimbing( Activity &idealActivity )
{
	if( !GetOuter()->IsClimbing() )
		return false;

	idealActivity = ACT_CLIMB_UP;
	return true;
}

bool UnitAnimState::HandleCrouching( Activity &idealActivity )
{
	if( !GetOuter()->IsCrouching() )
		return false;

	float flSpeed = GetOuterXYSpeed();

	if ( flSpeed > MOVING_MINIMUM_SPEED )
	{
		idealActivity = ACT_RUN_CROUCH;
	}
	else
	{
		idealActivity = ACT_CROUCH;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool UnitAnimState::HandleJumping( Activity &idealActivity )
{
	Vector vecVelocity;
	GetOuterAbsVelocity( vecVelocity );

	if ( m_bJumping )
	{
		if ( m_bFirstJumpFrame )
		{
			m_bFirstJumpFrame = false;
			RestartMainSequence();	// Reset the animation.
		}

		// Reset if we hit water and start swimming.
		if ( GetOuter()->GetWaterLevel() >= WL_Waist )
		{
			m_bJumping = false;
			RestartMainSequence();
		}
		// Don't check if he's on the ground for a sec.. sometimes the client still has the
		// on-ground flag set right when the message comes in.
		else if ( gpGlobals->curtime - m_flJumpStartTime > 0.2f )
		{
			if ( GetOuter()->GetFlags() & FL_ONGROUND )
			{
				m_bJumping = false;
				RestartMainSequence();

				if ( m_bNewJump )
				{
					//RestartGesture( GESTURE_SLOT_JUMP, ACT_MP_JUMP_LAND );					
				}
			}
		}

		// if we're still jumping
		if ( m_bJumping )
		{
			if ( m_bNewJump )
			{
				if ( gpGlobals->curtime - m_flJumpStartTime > 0.5 )
				{
					idealActivity = ACT_MP_JUMP_FLOAT;
				}
				else
				{
					idealActivity = ACT_MP_JUMP_START;
				}
			}
			else
			{
				idealActivity = ACT_MP_JUMP;
			}
		}
	}	

	if ( m_bJumping )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : Activity
//-----------------------------------------------------------------------------
Activity UnitAnimState::CalcMainActivity()
{
	Activity idealActivity = ACT_IDLE;

	if ( HandleClimbing( idealActivity ) ||
		HandleJumping( idealActivity ) ||
		HandleCrouching( idealActivity ) )
	{
		// intentionally blank
	}
	else
	{
		float flSpeed = GetOuterXYSpeed();

		if( m_bPlayFallActInAir && (GetFlags() & FL_ONGROUND) == 0 )
		{
			idealActivity = ACT_MP_JUMP_FLOAT;
		}
		else if ( flSpeed > MOVING_MINIMUM_SPEED )
		{
			idealActivity = ACT_RUN;
		}
	}

	return idealActivity;
}

void UnitAnimState::EndSpecificActivity()
{
	// NOTE: OnEndSpecificActivity might directly set a new specific main act, so check that.
	//		 Also prefer that one above the returned act (if any).
	m_bSpecMainUseCustomPlaybackrate = false;
	Activity iOldSpecificMainActivity = m_nSpecificMainActivity;
	m_nSpecificMainActivity = ACT_INVALID;
	Activity iNewSpecificMainActvity = OnEndSpecificActivity( iOldSpecificMainActivity );
	if( m_nSpecificMainActivity == ACT_INVALID )
		m_nSpecificMainActivity = iNewSpecificMainActvity;
}

void UnitAnimState::ComputeMainSequence()
{
	VPROF( "UnitAnimState::ComputeMainSequence" );

	CBaseAnimatingOverlay *pOuter = GetOuter();

	// Have our class or the mod-specific class determine what the current activity is.
	Activity idealActivity = CalcMainActivity();

#ifdef CLIENT_DLL
	Activity oldActivity = m_eCurrentMainSequenceActivity;
#endif
	
	// Store our current activity so the aim and fire layers know what to do.
	m_eCurrentMainSequenceActivity = idealActivity;

	// Hook to force playback of a specific requested full-body sequence
	if ( m_nSpecificMainActivity != ACT_INVALID )
	{
		if ( pOuter->GetSequenceActivity( pOuter->GetSequence() ) != m_nSpecificMainActivity )
		{
			pOuter->ResetSequence( SelectWeightedSequence( m_nSpecificMainActivity ) );
			if( pOuter->GetSequence() == -1 )
			{
				EndSpecificActivity();
			}
			return;
		}

		if ( !pOuter->IsSequenceFinished() )
			return;

		EndSpecificActivity();

		if( m_nSpecificMainActivity != ACT_INVALID )
		{
			pOuter->ResetSequence( SelectWeightedSequence( m_nSpecificMainActivity ) );
			if( pOuter->GetSequence() == -1 )
			{
				EndSpecificActivity();
			}
			return;
		}
		else
		{
			RestartMainSequence();
		}
	}

	// Export to our outer class..
	int animDesired = SelectWeightedSequence( TranslateActivity(idealActivity) );

#if !defined( HL1_CLIENT_DLL ) && !defined ( HL1_DLL )
	if ( pOuter->GetSequenceActivity( pOuter->GetSequence() ) == pOuter->GetSequenceActivity( animDesired ) )
		return;
#endif

	if ( animDesired < 0 )
		 animDesired = 0;

	pOuter->ResetSequence( animDesired );

#ifdef CLIENT_DLL
	if ( ShouldResetGroundSpeed( oldActivity, idealActivity ) )
	{
		ResetGroundSpeed();
	}
#endif
}





void UnitAnimState::UpdateAimSequenceLayers(
	float flCycle,
	int iFirstLayer,
	bool bForceIdle,
	CSequenceTransitioner *pTransitioner,
	float flWeightScale
	)
{
	float flAimSequenceWeight = 1;
	int iAimSequence = CalcAimLayerSequence( &flCycle, &flAimSequenceWeight, bForceIdle );
	if ( iAimSequence == -1 )
		iAimSequence = 0;

	// Feed the current state of the animation parameters to the sequence transitioner.
	// It will hand back either 1 or 2 animations in the queue to set, depending on whether
	// it's transitioning or not. We just dump those into the animation layers.
	pTransitioner->CheckForSequenceChange(
		m_pOuter->GetModelPtr(),
		iAimSequence,
		false,	// don't force transitions on the same anim
		true	// yes, interpolate when transitioning
		);

	pTransitioner->UpdateCurrent(
		m_pOuter->GetModelPtr(),
		iAimSequence,
		flCycle,
		GetOuter()->GetPlaybackRate(),
		gpGlobals->curtime
		);

	CAnimationLayer *pDest0 = m_pOuter->GetAnimOverlay( iFirstLayer );
	CAnimationLayer *pDest1 = m_pOuter->GetAnimOverlay( iFirstLayer+1 );

	if ( pTransitioner->m_animationQueue.Count() == 1 )
	{
		// If only 1 animation, then blend it in fully.
		CAnimationLayer *pSource0 = &pTransitioner->m_animationQueue[0];
		*pDest0 = *pSource0;
		
		pDest0->SetWeight( 1 );
		pDest1->SetWeight( 0 );
		pDest0->SetOrder( iFirstLayer );

#ifndef CLIENT_DLL
		pDest0->m_fFlags |= ANIM_LAYER_ACTIVE;
#endif
	}
	else if ( pTransitioner->m_animationQueue.Count() >= 2 )
	{
		// The first one should be fading out. Fade in the new one inversely.
		CAnimationLayer *pSource0 = &pTransitioner->m_animationQueue[0];
		CAnimationLayer *pSource1 = &pTransitioner->m_animationQueue[1];

		*pDest0 = *pSource0;
		*pDest1 = *pSource1;
		Assert( pDest0->GetWeight() >= 0.0f && pDest0->GetWeight() <= 1.0f );
		pDest1->SetWeight( 1 - pDest0->GetWeight() );	// This layer just mirrors the other layer's weight (one fades in while the other fades out).

		pDest0->SetOrder( iFirstLayer );
		pDest1->SetOrder( iFirstLayer+1 );

#ifndef CLIENT_DLL
		pDest0->m_fFlags |= ANIM_LAYER_ACTIVE;
		pDest1->m_fFlags |= ANIM_LAYER_ACTIVE;
#endif
	}
	
	pDest0->SetWeight( pDest0->GetWeight() * flWeightScale * flAimSequenceWeight );
	pDest0->SetWeight( clamp( (float)pDest0->GetWeight(), 0.0f, 1.0f ) );

	pDest1->SetWeight( pDest1->GetWeight() * flWeightScale * flAimSequenceWeight );
	pDest1->SetWeight( clamp( (float)pDest1->GetWeight(), 0.0f, 1.0f ) );

	pDest0->SetCycle( flCycle );
	pDest1->SetCycle( flCycle );
}


void UnitAnimState::OptimizeLayerWeights( int iFirstLayer, int nLayers )
{
	int i;

	// Find the total weight of the blended layers, not including the idle layer (iFirstLayer)
	float totalWeight = 0.0f;
	for ( i=1; i < nLayers; i++ )
	{
		CAnimationLayer *pLayer = m_pOuter->GetAnimOverlay( iFirstLayer+i );
		if ( pLayer->IsActive() && pLayer->GetWeight() > 0.0f )
		{
			totalWeight += pLayer->GetWeight();
		}
	}

	// Set the idle layer's weight to be 1 minus the sum of other layer weights
	CAnimationLayer *pLayer = m_pOuter->GetAnimOverlay( iFirstLayer );
	if ( pLayer->IsActive() && pLayer->GetWeight() > 0.0f )
	{
		float flWeight = 1.0f - totalWeight;
		flWeight = Max( flWeight, 0.0f );
		pLayer->SetWeight( flWeight );
	}

	// This part is just an optimization. Since we have the walk/run animations weighted on top of 
	// the idle animations, all this does is disable the idle animations if the walk/runs are at
	// full weighting, which is whenever a guy is at full speed.
	//
	// So it saves us blending a couple animation layers whenever a guy is walking or running full speed.
	int iLastOne = -1;
	for ( i=0; i < nLayers; i++ )
	{
		CAnimationLayer *pLayer = m_pOuter->GetAnimOverlay( iFirstLayer+i );
		if ( pLayer->IsActive() && pLayer->GetWeight() > 0.99 )
			iLastOne = i;
	}

	if ( iLastOne != -1 )
	{
		for ( int i=iLastOne-1; i >= 0; i-- )
		{
			CAnimationLayer *pLayer = m_pOuter->GetAnimOverlay( iFirstLayer+i );
#ifdef CLIENT_DLL 
			pLayer->SetOrder( CBaseAnimatingOverlay::MAX_OVERLAYS );
#else
			pLayer->m_nOrder.Set( CBaseAnimatingOverlay::MAX_OVERLAYS );
			pLayer->m_fFlags = 0;
#endif
		}
	}
}

bool UnitAnimState::ShouldBlendAimSequenceToIdle()
{
	Activity act = GetCurrentMainSequenceActivity();

	return (act == ACT_RUN || act == ACT_WALK || act == ACT_RUNTOIDLE || act == ACT_RUN_CROUCH);
}

void UnitAnimState::ComputeAimSequence()
{
	VPROF( "UnitAnimState::ComputeAimSequence" );

	// Synchronize the lower and upper body cycles.
	float flCycle = GetOuter()->GetCycle();

	// Figure out the new cycle time.
	UpdateAimSequenceLayers( flCycle, AIMSEQUENCE_LAYER, true, &m_IdleSequenceTransitioner, 1 );
	
	if ( ShouldBlendAimSequenceToIdle() )
	{
		// What we do here is blend between the idle upper body animation (like where he's got the dual elites
		// held out in front of him but he's not moving) and his walk/run/crouchrun upper body animation,
		// weighting it based on how fast he's moving. That way, when he's moving slowly, his upper 
		// body doesn't jiggle all around.
		bool bIsMoving;
		float flPlaybackRate = CalcMovementPlaybackRate( &bIsMoving );
		if ( bIsMoving )
			UpdateAimSequenceLayers( flCycle, AIMSEQUENCE_LAYER+2, false, &m_SequenceTransitioner, flPlaybackRate );
	}

	OptimizeLayerWeights( AIMSEQUENCE_LAYER, NUM_AIMSEQUENCE_LAYERS );
}


int UnitAnimState::CalcSequenceIndex( const char *pBaseName, ... )
{
	char szFullName[512];
	va_list marker;
	va_start( marker, pBaseName );
	V_vsnprintf( szFullName, sizeof( szFullName ), pBaseName, marker );
	va_end( marker );
	int iSequence = GetOuter()->LookupSequence( szFullName );
	
	// Show warnings if we can't find anything here.
	if ( iSequence == -1 )
	{
		static CUtlDict<int,int> dict;
		if ( dict.Find( szFullName ) == -1 )
		{
			dict.Insert( szFullName, 0 );
			Warning( "CalcSequenceIndex: can't find '%s'.\n", szFullName );
		}

		iSequence = 0;
	}

	return iSequence;
}



void UnitAnimState::UpdateInterpolators()
{
	VPROF( "UnitAnimState::UpdateInterpolators" );

	// First, figure out their current max speed based on their current activity.
	float flCurMaxSpeed = GetCurrentMaxGroundSpeed();
	m_flMaxGroundSpeed = flCurMaxSpeed;
}


float UnitAnimState::GetInterpolatedGroundSpeed()
{
	return m_flMaxGroundSpeed;
}


float UnitAnimState::CalcMovementPlaybackRate( bool *bIsMoving )
{
	// Determine ideal playback rate
	Vector vel;
	GetOuterAbsVelocity( vel );

	float speed = vel.Length2D();
	bool isMoving = ( speed > MOVING_MINIMUM_SPEED );

	*bIsMoving = false;
	float flReturnValue = 1;

	if ( isMoving && CanTheUnitMove() )
	{
		float flGroundSpeed = GetInterpolatedGroundSpeed();
		if ( flGroundSpeed < 0.001f )
		{
			flReturnValue = 0.01;
		}
		else
		{
			// Note this gets set back to 1.0 if sequence changes due to ResetSequenceInfo below
			flReturnValue = speed / flGroundSpeed;
			flReturnValue = clamp( flReturnValue, 0.01, 10 );	// don't go nuts here.
		}
		*bIsMoving = true;
	}
	
	return flReturnValue;
}

void UnitAnimState::SetCustomSpecificActPlaybackRate( float playbackrate )
{
	m_bSpecMainUseCustomPlaybackrate = true;
	m_fSpecMainPlaybackRate = playbackrate;
}

#ifdef ENABLE_PYTHON
void UnitAnimState::SetActivityMap( boost::python::object activitymap )
{
	if( activitymap.ptr() == Py_None )
	{
		m_pActivityMap = NULL;
		m_pyActivityMap = boost::python::object();
		return;
	}
	m_pActivityMap = boost::python::extract<TranslateActivityMap *>(activitymap);
	m_pyActivityMap = activitymap;
}
#endif // ENABLE_PYTHON

Activity UnitAnimState::TranslateActivity( Activity actDesired )
{ 
	if( m_bUseCombatState )
	{
		if( (m_bCombatStateIfEnemy && GetOuter()->GetEnemy()) || m_fCombatStateTime > gpGlobals->curtime )
		{
			if( actDesired == ACT_IDLE )
				actDesired = ACT_IDLE_AIM_AGITATED; // Meh, no ACT_IDLE_AIM :(
			else if( actDesired == ACT_WALK )
				actDesired = ACT_WALK_AIM;
			else if( actDesired == ACT_RUN )
				actDesired = ACT_RUN_AIM;
			else if( actDesired == ACT_CROUCH )
				actDesired = ACT_CROUCHIDLE_AIM_STIMULATED; // Meh, no ACT_IDLE_AIM :(
			else if( actDesired == ACT_WALK_CROUCH )
				actDesired = ACT_WALK_CROUCH_AIM;
			else if( actDesired == ACT_RUN_CROUCH )
				actDesired = ACT_RUN_CROUCH_AIM;
		}
	}

	if( m_pActivityMap )
	{
		int idx = m_pActivityMap->m_translateActivityMap.Find(actDesired);
		if( m_pActivityMap->m_translateActivityMap.IsValidIndex(idx) )
			return m_pActivityMap->m_translateActivityMap[idx];
	}
	return actDesired; 
}

void UnitAnimState::SetAimLayerSequence( const char *pAimLayerSequence )
{
	m_sAimLayerSequence = AllocPooledString(pAimLayerSequence);
}

const char *UnitAnimState::GetAimLayerSequence()
{
	return STRING(m_sAimLayerSequence);
}

bool UnitAnimState::CanTheUnitMove()
{
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Override the default, because hl2mp models don't use moveX
// Input  :  - 
// Output : float
//-----------------------------------------------------------------------------
float UnitAnimState::GetCurrentMaxGroundSpeed()
{
	CStudioHdr *pStudioHdr = GetOuter()->GetModelPtr();

	if ( pStudioHdr == NULL )
		return 1.0f;

	float speed;
	if ( m_AnimConfig.m_LegAnimType == LEGANIM_8WAY )
	{
		if ( m_iMoveYaw < 0 )
			return 1.0;

		float prevY = GetOuter()->GetPoseParameter( m_iMoveYaw );

		float d = sqrt( prevY * prevY );
		float newY;
		if ( d == 0.0 )
		{ 
			newY = 0.0;
		}
		else
		{
			newY = prevY / d;
		}

		GetOuter()->SetPoseParameter( pStudioHdr, m_iMoveYaw, newY );

		speed = GetOuter()->GetSequenceGroundSpeed( GetOuter()->GetSequence() );

		GetOuter()->SetPoseParameter( pStudioHdr, m_iMoveYaw, prevY );
	}
	else
	{
		if ( m_iMoveX < 0 || m_iMoveY < 0 )
			return 1.0;

		float prevX = GetOuter()->GetPoseParameter( m_iMoveX );
		float prevY = GetOuter()->GetPoseParameter( m_iMoveY );

		float d = sqrt( prevX * prevX + prevY * prevY );
		float newY, newX;
		if ( d == 0.0 )
		{ 
			newX = 1.0;
			newY = 0.0;
		}
		else
		{
			newX = prevX / d;
			newY = prevY / d;
		}

		GetOuter()->SetPoseParameter( pStudioHdr, m_iMoveX, newX );
		GetOuter()->SetPoseParameter( pStudioHdr, m_iMoveY, newY );

		speed = GetOuter()->GetSequenceGroundSpeed( GetOuter()->GetSequence() );

		GetOuter()->SetPoseParameter( pStudioHdr, m_iMoveX, prevX );
		GetOuter()->SetPoseParameter( pStudioHdr, m_iMoveY, prevY );
	}

	return speed;
}

void UnitAnimState::ComputePlaybackRate()
{
	VPROF( "UnitAnimState::ComputePlaybackRate" );
	if( m_bSpecMainUseCustomPlaybackrate )
	{
		GetOuter()->SetPlaybackRate( m_fSpecMainPlaybackRate );
	}
	else if ( m_AnimConfig.m_LegAnimType != LEGANIM_9WAY && m_AnimConfig.m_LegAnimType != LEGANIM_8WAY )
	{
		// When using a 9-way blend, playback rate is always 1 and we just scale the pose params
		// to speed up or slow down the animation.
		bool bIsMoving;
		float flRate = CalcMovementPlaybackRate( &bIsMoving );
		if ( bIsMoving )
			GetOuter()->SetPlaybackRate( flRate );
		else
			GetOuter()->SetPlaybackRate( 1 );
	}
	else
	{
		// Default to 1
		GetOuter()->SetPlaybackRate( 1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : dt - 
//-----------------------------------------------------------------------------
void UnitAnimState::EstimateYaw()
{
	Vector est_velocity;
	GetOuterAbsVelocity( est_velocity );

	float flLength = est_velocity.Length2D();
	if ( flLength > MOVING_MINIMUM_SPEED )
	{
		m_flGaitYaw = atan2( est_velocity[1], est_velocity[0] );
		m_flGaitYaw = RAD2DEG( m_flGaitYaw );
		m_flGaitYaw = AngleNormalize( m_flGaitYaw );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Override for backpeddling
// Input  : dt - 
//-----------------------------------------------------------------------------
void UnitAnimState::ComputePoseParam_MoveYaw( CStudioHdr *pStudioHdr )
{
	VPROF( "UnitAnimState::ComputePoseParam_MoveYaw" );

	//Matt: Goldsrc style animations need to not rotate the model
	if ( m_AnimConfig.m_LegAnimType == LEGANIM_GOLDSRC )
	{
#ifndef CLIENT_DLL
		//Adrian: Make the model's angle match the legs so the hitboxes match on both sides.
		GetOuter()->SetLocalAngles( QAngle( 0, m_flCurrentFeetYaw, 0 ) );
#endif
	}

	// If using goldsrc-style animations where he's moving in the direction that his feet are facing,
	// we don't use move yaw.
	if ( m_AnimConfig.m_LegAnimType != LEGANIM_9WAY && m_AnimConfig.m_LegAnimType != LEGANIM_8WAY )
		return;

	// view direction relative to movement
	float flYaw;	 

	EstimateYaw();

	float ang = m_flEyeYaw;
	if ( ang > 180.0f )
	{
		ang -= 360.0f;
	}
	else if ( ang < -180.0f )
	{
		ang += 360.0f;
	}

	// calc side to side turning
	flYaw = ang - m_flGaitYaw;
	// Invert for mapping into 8way blend
	flYaw = -flYaw;
	flYaw = flYaw - (int)(flYaw / 360) * 360;

	if (flYaw < -180)
	{
		flYaw = flYaw + 360;
	}
	else if (flYaw > 180)
	{
		flYaw = flYaw - 360;
	}

	
	if ( m_AnimConfig.m_LegAnimType == LEGANIM_9WAY )
	{
#ifndef CLIENT_DLL
		//Adrian: Make the model's angle match the legs so the hitboxes match on both sides.
		// FIXME: Can't do this. Then we also need to do it for 8way, since the unit base does send the angles from the server.
		//GetOuter()->SetLocalAngles( QAngle( 0, m_flCurrentFeetYaw, 0 ) );
#endif
		if ( m_iMoveX < 0 || m_iMoveY < 0 )
			return;

		bool bIsMoving;
		float flPlaybackRate = CalcMovementPlaybackRate( &bIsMoving );

		// Setup the 9-way blend parameters based on our speed and direction.
		Vector2D vCurMovePose( 0, 0 );

		if ( bIsMoving )
		{
			vCurMovePose.x = cos( DEG2RAD( flYaw ) ) * flPlaybackRate;
			vCurMovePose.y = -sin( DEG2RAD( flYaw ) ) * flPlaybackRate;
		}

		GetOuter()->SetPoseParameter( pStudioHdr, m_iMoveX, vCurMovePose.x );
		GetOuter()->SetPoseParameter( pStudioHdr, m_iMoveY, m_bFlipMoveY ? -vCurMovePose.y : vCurMovePose.y );

		m_vLastMovePose = vCurMovePose;
	}
	else
	{
		if ( m_iMoveYaw >= 0 )
		{
			GetOuter()->SetPoseParameter( pStudioHdr, m_iMoveYaw, flYaw );
			m_flLastMoveYaw = flYaw;

			// Now blend in his idle animation.
			// This makes the 8-way blend act like a 9-way blend by blending to 
			// an idle sequence as he slows down.
#if defined(CLIENT_DLL)
#ifndef HL2WARS_DLL // FIXME: Gives problems with the specific sequences
			bool bIsMoving;
			CAnimationLayer *pLayer = GetOuter()->GetAnimOverlay( MAIN_IDLE_SEQUENCE_LAYER );
			
			pLayer->SetWeight( 1 - CalcMovementPlaybackRate( &bIsMoving ) );
			if ( !bIsMoving )
			{
				pLayer->SetWeight( 1 );
			}

			if ( ShouldChangeSequences() )
			{
				// Whenever this layer stops blending, we can choose a new idle sequence to blend to, so he 
				// doesn't always use the same idle.
				if ( pLayer->GetWeight() < 0.02f || m_iCurrent8WayIdleSequence == -1 )
				{
					m_iCurrent8WayIdleSequence = GetOuter()->SelectWeightedSequence( ACT_IDLE );
					m_iCurrent8WayCrouchIdleSequence = GetOuter()->SelectWeightedSequence( ACT_CROUCHIDLE );
				}

				if ( m_eCurrentMainSequenceActivity == ACT_CROUCHIDLE || m_eCurrentMainSequenceActivity == ACT_RUN_CROUCH )
					pLayer->SetSequence( m_iCurrent8WayCrouchIdleSequence );
				else
					pLayer->SetSequence( m_iCurrent8WayIdleSequence );
			}
			
			pLayer->SetPlaybackRate( 1 );
			pLayer->SetCycle( pLayer->GetCycle() + m_pOuter->GetSequenceCycleRate( pStudioHdr, pLayer->GetSequence() ) * GetAnimTimeInterval() );
			pLayer->SetCycle( fmod( pLayer->GetCycle(), 1 ) );
			pLayer->SetOrder( MAIN_IDLE_SEQUENCE_LAYER );
#endif // HL2WARS_DLL
#endif
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnitAnimState::ComputePoseParam_BodyPitch( CStudioHdr *pStudioHdr )
{
	VPROF( "UnitAnimState::ComputePoseParam_BodyPitch" );

	// Get pitch from v_angle
	float flPitch = m_flEyePitch;
	if ( flPitch > 180.0f )
	{
		flPitch -= 360.0f;
	}
	flPitch = clamp( flPitch, -90, 90 );

	// See if we have a blender for pitch
	if ( m_iBodyPitch < 0 )
		return;

	if( m_AnimConfig.m_bInvertPoseParameters )
		flPitch *= -1;

	GetOuter()->SetPoseParameter( pStudioHdr, m_iBodyPitch, flPitch );
	g_flLastBodyPitch = flPitch;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : goal - 
//			maxrate - 
//			dt - 
//			current - 
// Output : int
//-----------------------------------------------------------------------------
int UnitAnimState::ConvergeAngles( float goal,float maxrate, float maxgap, float dt, float& current )
{
	int direction = TURN_NONE;

	float anglediff = goal - current;
	anglediff = AngleNormalize( anglediff );
	
	float anglediffabs = fabs( anglediff );

	float scale = 1.0f;
	if ( anglediffabs <= FADE_TURN_DEGREES )
	{
		scale = anglediffabs / FADE_TURN_DEGREES;
		// Always do at least a bit of the turn ( 1% )
		scale = clamp( scale, 0.01f, 1.0f );
	}

	float maxmove = maxrate * dt * scale;

	if ( anglediffabs > maxgap )
	{
		// gap is too big, jump
		//maxmove = (anglediffabs - maxgap);
		float flTooFar = Min( anglediffabs - maxgap, maxmove * 5 );
		if ( anglediff > 0 )
		{
			current += flTooFar;
		}
		else
		{
			current -= flTooFar;
		}
		current = AngleNormalize( current );
		anglediff = goal - current;
		anglediff = AngleNormalize( anglediff );
		anglediffabs = fabs( anglediff );
		//Msg( "jumped = %f\n", flTooFar );
	}

	if ( anglediffabs < maxmove )
	{
		// we are close enought, just set the final value
		current = goal;
	}
	else
	{
		// adjust value up or down
		if ( anglediff > 0 )
		{
			current += maxmove;
			direction = TURN_LEFT;
		}
		else
		{
			current -= maxmove;
			direction = TURN_RIGHT;
		}
	}

	current = AngleNormalize( current );

	return direction;
}

void UnitAnimState::ComputePoseParam_BodyYaw()
{
	VPROF( "UnitAnimState::ComputePoseParam_BodyYaw" );

	// Find out which way he's running (m_flEyeYaw is the way he's looking).
	Vector vel;
	GetOuterAbsVelocity( vel );
	bool bIsMoving = vel.Length2D() > MOVING_MINIMUM_SPEED;
	float flMaxGap = m_AnimConfig.m_flMaxBodyYawDegrees;

	// TOOD: Add support for something like this
#if 0
	if ( m_pOuter && m_pOuter->GetCurrentMeleeAttack() )		// don't allow upper body turning when melee attacking
	{
		flMaxGap = 0.0f;
	}
#endif // 0

	// If we just initialized this guy (maybe he just came into the PVS), then immediately
	// set his feet in the right direction, otherwise they'll spin around from 0 to the 
	// right direction every time someone switches spectator targets.
	if ( !m_bCurrentFeetYawInitialized )
	{
		m_bCurrentFeetYawInitialized = true;
		m_flGoalFeetYaw = m_flCurrentFeetYaw = m_flEyeYaw;
		m_flLastTurnTime = 0.0f;
	}
	else if ( bIsMoving )
	{
		// player is moving, feet yaw = aiming yaw
		if ( m_AnimConfig.m_LegAnimType == LEGANIM_9WAY || m_AnimConfig.m_LegAnimType == LEGANIM_8WAY )
		{
			// His feet point in the direction his eyes are, but they can run in any direction.
			m_flGoalFeetYaw = m_flEyeYaw;
		}
		else
		{
			m_flGoalFeetYaw = RAD2DEG( atan2( vel.y, vel.x ) );

			// If he's running backwards, flip his feet backwards.
			Vector vEyeYaw( cos( DEG2RAD( m_flEyeYaw ) ), sin( DEG2RAD( m_flEyeYaw ) ), 0 );
			Vector vFeetYaw( cos( DEG2RAD( m_flGoalFeetYaw ) ), sin( DEG2RAD( m_flGoalFeetYaw ) ), 0 );
			if ( vEyeYaw.Dot( vFeetYaw ) < -0.01 )
			{
				m_flGoalFeetYaw += 180;
			}
		}

	}
	else if ( (gpGlobals->curtime - m_flLastTurnTime) > m_flFaceFrontTime )
	{
		// player didn't move & turn for quite some time
		m_flGoalFeetYaw = m_flEyeYaw;
	}
	else
	{
		// If he's rotated his view further than the model can turn, make him face forward.
		float flDiff = AngleNormalize(  m_flGoalFeetYaw - m_flEyeYaw );

		if ( fabs(flDiff) > flMaxGap )
		{
			m_flGoalFeetYaw = m_flEyeYaw;
			//flMaxGap = 180.0f / GetAnimTimeInterval();
		}
	}

	m_flGoalFeetYaw = AngleNormalize( m_flGoalFeetYaw );

	if ( m_flCurrentFeetYaw != m_flGoalFeetYaw )
	{
#ifdef CLIENT_DLL
		ConvergeAngles( m_flGoalFeetYaw, m_flFeetYawRate, flMaxGap,
			GetAnimTimeInterval(), m_flCurrentFeetYaw );
#else
		if( m_pOuter->GetCommander() )
		{
			ConvergeAngles( m_flGoalFeetYaw, m_flFeetYawRate, flMaxGap,
				GetAnimTimeInterval(), m_flCurrentFeetYaw );
		}
		else
		{
			// FIXME: Retrieve correct interval!
			ConvergeAngles( m_flGoalFeetYaw, m_flFeetYawRate, flMaxGap,
				0.1f, m_flCurrentFeetYaw );
		}
#endif // CLIENT_DLL
		m_flLastTurnTime = gpGlobals->curtime;
	}

	float flCurrentTorsoYaw = AngleNormalize( m_flEyeYaw - m_flCurrentFeetYaw );

	// Rotate entire body into position
	m_angRender[YAW] = m_flCurrentFeetYaw;
	m_angRender[PITCH] = m_angRender[ROLL] = 0;

	SetOuterBodyYaw( flCurrentTorsoYaw );
	g_flLastBodyYaw = flCurrentTorsoYaw;
}



float UnitAnimState::SetOuterBodyYaw( float flValue )
{
	if ( m_iBodyYaw < 0 )
	{
		return 0;
	}

	if( m_AnimConfig.m_bInvertPoseParameters )
		flValue *= -1;
	if( m_AnimConfig.m_bBodyYawNormalized )
		flValue = flValue / m_AnimConfig.m_flMaxBodyYawDegrees;

	SetOuterPoseParameter( m_iBodyYaw, flValue );
	return flValue;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UnitAnimState::ComputePoseParam_Lean()
{
	if ( m_iLeanYaw >= 0 )
	{
		float targetLean = GetOuter()->GetPoseParameter( m_iMoveY ) * 30.0f;
		float curLean = GetOuter()->GetPoseParameter( m_iLeanYaw );
		if( curLean < targetLean )
			curLean += Min(fabs(targetLean-curLean), GetAnimTimeInterval()*15.0f);
		else
			curLean -= Min(fabs(targetLean-curLean), GetAnimTimeInterval()*15.0f);
		SetOuterPoseParameter( m_iLeanYaw, curLean );
	}

	if( m_iLeanPitch >= 0 )
	{
		float targetLean = GetOuter()->GetPoseParameter( m_iMoveX ) * -30.0f;
		float curLean = GetOuter()->GetPoseParameter( m_iLeanPitch );
		if( curLean < targetLean )
			curLean += Min(fabs(targetLean-curLean), GetAnimTimeInterval()*15.0f);
		else
			curLean -= Min(fabs(targetLean-curLean), GetAnimTimeInterval()*15.0f);
		SetOuterPoseParameter( m_iLeanPitch, curLean );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : activity - 
// Output : Activity
//-----------------------------------------------------------------------------
Activity UnitAnimState::BodyYawTranslateActivity( Activity activity )
{
	// Not even standing still, sigh
	if ( activity != ACT_IDLE )
		return activity;

	// Not turning
	switch ( m_nTurningInPlace )
	{
	default:
	case TURN_NONE:
		return activity;
	case TURN_RIGHT:
	case TURN_LEFT:
		return mp_ik.GetBool() ? ACT_TURN : activity;
	}

	Assert( 0 );
	return activity;
}

const QAngle& UnitAnimState::GetRenderAngles()
{
	return m_angRender;
}

void UnitAnimState::UpdateLayerSequenceGeneric( int iLayer, bool &bEnabled,
					float &flCurCycle, int &iSequence, bool bWaitAtEnd,
					float fBlendIn, float fBlendOut, bool bMoveBlend, float fPlaybackRate, bool bUpdateCycle /* = true */ )
{
	if ( !bEnabled )
		return;

	CStudioHdr *hdr = GetOuter()->GetModelPtr();
	if ( !hdr )
		return;

	if ( iSequence < 0 || iSequence >= hdr->GetNumSeq() )
		return;

	// Increment the fire sequence's cycle.
	if ( bUpdateCycle )
	{
		flCurCycle += m_pOuter->GetSequenceCycleRate( hdr, iSequence ) * GetAnimTimeInterval() * fPlaybackRate;
	}

	// temp: if the sequence is looping, don't override it - we need better handling of looping anims, 
	//  especially in misc layer from melee (right now the same melee attack is looped manually in asw_melee_system.cpp)
	bool bLooping = m_pOuter->IsSequenceLooping( hdr, iSequence );

	if ( flCurCycle > 1 && !bLooping )
	{
		/*if ( iLayer == RELOADSEQUENCE_LAYER )
		{
			m_bPlayingEmoteGesture = false;
			m_bReloading = false;
		}*/
		if ( bWaitAtEnd )
		{
			flCurCycle = 1;
		}
		else
		{
			// Not firing anymore.
			bEnabled = false;
			iSequence = 0;
			return;
		}
	}

	// if this animation should blend out as we move, then check for dropping it completely since we're moving too fast
	float speed = 0;
	if (bMoveBlend)
	{
		Vector vel;
		GetOuterAbsVelocity( vel );

		float speed = vel.Length2D();

		if (speed > 50)
		{
			bEnabled = false;
			iSequence = 0;
			return;
		}
	}

	// Now dump the state into its animation layer.
	CAnimationLayer *pLayer = m_pOuter->GetAnimOverlay( iLayer );

	pLayer->SetCycle( flCurCycle );
	pLayer->SetSequence( iSequence );

	pLayer->SetPlaybackRate( fPlaybackRate );
	pLayer->SetWeight( 1.0f );
	/*if (iLayer == RELOADSEQUENCE_LAYER)
	{
		// blend this layer in and out for smooth reloading
		if (flCurCycle < fBlendIn && fBlendIn>0)
		{
			pLayer->SetWeight( clamp<float>(flCurCycle / fBlendIn,
				0.001f, 1.0f) );
		}
		else if (flCurCycle >= (1.0f - fBlendOut) && fBlendOut>0)
		{
			pLayer->SetWeight( clamp<float>((1.0f - flCurCycle) / fBlendOut,
				0.001f, 1.0f) );
		}
		else
		{
			pLayer->SetWeight( 1.0f );
		}
	}
	else*/
	{
		pLayer->SetWeight( 1.0f );			
	}
	if (bMoveBlend)		
	{
		// blend the animation out as we move faster
		if (speed <= 50)			
			pLayer->SetWeight( pLayer->GetWeight() * (50.0f - speed) / 50.0f );
	}

#ifndef CLIENT_DLL
	pLayer->m_fFlags |= ANIM_LAYER_ACTIVE;
#endif
	pLayer->SetOrder( iLayer );
}

// -----------------------------------------------------------------------------
void UnitAnimState::AnimStateLog( const char *pMsg, ... )
{
	// Format the string.
	char str[4096];
	va_list marker;
	va_start( marker, pMsg );
	V_vsnprintf( str, sizeof( str ), pMsg, marker );
	va_end( marker );

	// Log it?	
	if ( showanimstate_log.GetInt() == 1 || showanimstate_log.GetInt() == 3 )
	{
		Msg( "%s", str );
	}

	if ( showanimstate_log.GetInt() > 1 )
	{
#ifdef CLIENT_DLL
		const char *fname = "AnimStateClient.log";
#else
		const char *fname = "AnimStateServer.log";
#endif
		static FileHandle_t hFile = filesystem->Open( fname, "wt" );
		filesystem->FPrintf( hFile, "%s", str );
		filesystem->Flush( hFile );
	}
}


// -----------------------------------------------------------------------------
void UnitAnimState::AnimStatePrintf( int iLine, const char *pMsg, ... )
{
	// Format the string.
	char str[4096];
	va_list marker;
	va_start( marker, pMsg );
	V_vsnprintf( str, sizeof( str ), pMsg, marker );
	va_end( marker );

	// Show it with Con_NPrintf.
	engine->Con_NPrintf( iLine, "%s", str );

	// Log it.
	AnimStateLog( "%s\n", str );
}


// -----------------------------------------------------------------------------
void UnitAnimState::DebugShowAnimState( int iStartLine )
{
	Vector vOuterVel;
	GetOuterAbsVelocity( vOuterVel );

	int iLine = iStartLine;
	AnimStatePrintf( iLine++, "main: %s(%d), cycle: %.2f cyclerate: %.2f playbackrate: %.2f onground: %d\n", 
		GetSequenceName( m_pOuter->GetModelPtr(), m_pOuter->GetSequence() ), 
		m_pOuter->GetSequence(),
		m_pOuter->GetCycle(), 
		m_pOuter->GetSequenceCycleRate(m_pOuter->GetModelPtr(), m_pOuter->GetSequence()),
		m_pOuter->GetPlaybackRate(),
		m_pOuter->GetGroundEntity() != NULL
		);

	if ( m_AnimConfig.m_LegAnimType == LEGANIM_8WAY )
	{
		CAnimationLayer *pLayer = m_pOuter->GetAnimOverlay( MAIN_IDLE_SEQUENCE_LAYER );

		AnimStatePrintf( iLine++, "idle: %s, weight: %.2f\n",
			GetSequenceName( m_pOuter->GetModelPtr(), pLayer->GetSequence() ), 
			(float)pLayer->GetWeight() );
	}

	for ( int i=0; i < m_pOuter->GetNumAnimOverlays()-1; i++ )
	{
		CAnimationLayer *pLayer = m_pOuter->GetAnimOverlay( AIMSEQUENCE_LAYER + i );
#ifdef CLIENT_DLL
		AnimStatePrintf( iLine++, "%s(%d), weight: %.2f, cycle: %.2f, order (%d), aim (%d)", 
			!pLayer->IsActive() ? "-- ": (pLayer->GetSequence() == 0 ? "-- " : (showanimstate_activities.GetBool()) ? GetSequenceActivityName( m_pOuter->GetModelPtr(), pLayer->GetSequence() ) : GetSequenceName( m_pOuter->GetModelPtr(), pLayer->GetSequence() ) ), 
			!pLayer->IsActive() ? 0 : (int)pLayer->GetSequence(), 
			!pLayer->IsActive() ? 0 : (float)pLayer->GetWeight(), 
			!pLayer->IsActive() ? 0 : (float)pLayer->GetCycle(), 
			!pLayer->IsActive() ? 0 : (int)pLayer->GetOrder(),
			i
			);
#else
		AnimStatePrintf( iLine++, "%s(%d), flags (%d), weight: %.2f, cycle: %.2f, order (%d), aim (%d)", 
			!pLayer->IsActive() ? "-- " : ( pLayer->GetSequence() == 0 ? "-- " : (showanimstate_activities.GetBool()) ? GetSequenceActivityName( m_pOuter->GetModelPtr(), pLayer->GetSequence() ) : GetSequenceName( m_pOuter->GetModelPtr(), pLayer->GetSequence() ) ), 
			!pLayer->IsActive() ? 0 : (int)pLayer->GetSequence(), 
			!pLayer->IsActive() ? 0 : (int)pLayer->m_fFlags,// Doesn't exist on client
			!pLayer->IsActive() ? 0 : (float)pLayer->GetWeight(), 
			!pLayer->IsActive() ? 0 : (float)pLayer->GetCycle(), 
			!pLayer->IsActive() ? 0 : (int)pLayer->m_nOrder,
			i
			);
#endif
	}

	AnimStatePrintf( iLine++, "vel: %.2f, time: %.2f, max: %.2f, animspeed: %.2f", 
		vOuterVel.Length2D(), gpGlobals->curtime, GetInterpolatedGroundSpeed(), m_pOuter->GetSequenceGroundSpeed(m_pOuter->GetSequence()) );
	
	if ( m_AnimConfig.m_LegAnimType == LEGANIM_8WAY )
	{
		AnimStatePrintf( iLine++, "ent yaw: %.2f, body_yaw: %.2f, move_yaw: %.2f, gait_yaw: %.2f, body_pitch: %.2f", 
			m_angRender[YAW], g_flLastBodyYaw, m_flLastMoveYaw, m_flGaitYaw, g_flLastBodyPitch );
	}
	else
	{
		AnimStatePrintf( iLine++, "ent yaw: %.2f, body_yaw: %.2f, body_pitch: %.2f, move_x: %.2f, move_y: %.2f", 
			m_angRender[YAW], g_flLastBodyYaw, g_flLastBodyPitch, m_vLastMovePose.x, m_vLastMovePose.y );
	}

	// Draw a red triangle on the ground for the eye yaw.
	float flBaseSize = 10;
	float flHeight = 80;
	Vector vBasePos = GetOuter()->GetAbsOrigin() + Vector( 0, 0, 3 );
	QAngle angles( 0, 0, 0 );
	angles[YAW] = m_flEyeYaw;
	Vector vForward, vRight, vUp;
	AngleVectors( angles, &vForward, &vRight, &vUp );
	debugoverlay->AddTriangleOverlay( vBasePos+vRight*flBaseSize/2, vBasePos-vRight*flBaseSize/2, vBasePos+vForward*flHeight, 255, 0, 0, 255, false, 0.01 );

	// Draw a blue triangle on the ground for the body yaw.
	angles[YAW] = m_angRender[YAW];
	AngleVectors( angles, &vForward, &vRight, &vUp );
	debugoverlay->AddTriangleOverlay( vBasePos+vRight*flBaseSize/2, vBasePos-vRight*flBaseSize/2, vBasePos+vForward*flHeight, 0, 0, 255, 255, false, 0.01 );

}

// -----------------------------------------------------------------------------
void UnitAnimState::DebugShowAnimStateFull( int iStartLine )
{
	AnimStateLog( "----------------- frame %d -----------------\n", gpGlobals->framecount );

	DebugShowAnimState( iStartLine );

	AnimStateLog( "--------------------------------------------\n\n" );
}

// -----------------------------------------------------------------------------

