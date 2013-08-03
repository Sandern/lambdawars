//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
//=============================================================================//

#ifndef UNIT_ANIMSTATE_H
#define UNIT_ANIMSTATE_H
#ifdef _WIN32
#pragma once
#endif


#include "iplayeranimstate.h"
#include "unit_baseanimstate.h"
#include "studio.h"
#include "sequence_transitioner.h"

#ifdef ENABLE_PYTHON
	#include "srcpy_boostpython.h"
#endif // ENABLE_PYTHON

#ifdef CLIENT_DLL
	class C_BaseAnimatingOverlay;
	#define CBaseAnimatingOverlay C_BaseAnimatingOverlay
#else
	class CBaseAnimatingOverlay;
#endif

// If a guy is moving slower than this, then he's considered to not be moving
// (so he goes to his idle animation at full playback rate rather than his walk 
// animation at low playback rate).
//#define MOVING_MINIMUM_SPEED 0.5f
#define MOVING_MINIMUM_SPEED 10.0f

#define MAIN_IDLE_SEQUENCE_LAYER 0	// For 8-way blended models, this layer blends an idle on top of the run/walk animation to simulate a 9-way blend.
									// For 9-way blended models, we don't use this layer.

#define AIMSEQUENCE_LAYER		1	// Aim sequence uses layers 0 and 1 for the weapon idle animation (needs 2 layers so it can blend).
#define NUM_AIMSEQUENCE_LAYERS	4	// Then it uses layers 2 and 3 to blend in the weapon run/walk/crouchwalk animation.

#define FIRESEQUENCE_LAYER		(AIMSEQUENCE_LAYER+NUM_AIMSEQUENCE_LAYERS)
#define RELOADSEQUENCE_LAYER	(FIRESEQUENCE_LAYER + 1)
#define NUM_LAYERS_WANTED		(RELOADSEQUENCE_LAYER + 1)

class TranslateActivityMap
{
public:
	TranslateActivityMap() {SetDefLessFunc( m_translateActivityMap );}
	TranslateActivityMap( TranslateActivityMap &activitymap );
#ifdef ENABLE_PYTHON
	TranslateActivityMap( bp::dict d );
	TranslateActivityMap( TranslateActivityMap &activitymap, bp::dict d );

	void AddTranslations( bp::dict d );
#endif // ENABLE_PYTHON
	void AddTranslation(Activity act, Activity act_translated) { m_translateActivityMap.Insert(act, act_translated); }

	void DebugPrint();

	CUtlMap<Activity, Activity> m_translateActivityMap;
};

// Everyone who derives from UnitAnimState gets to fill in this info
// to drive how the animation state is generated.
class UnitAnimConfig
{
public:
	UnitAnimConfig() {} 
	UnitAnimConfig(float maxbodyyawdegrees, bool bodyyawnormalized=false, LegAnimType_t leganimtype=LEGANIM_8WAY, 
		bool useaimsequences=false, bool invertposeparameters=false) :
		m_flMaxBodyYawDegrees(maxbodyyawdegrees), m_bBodyYawNormalized(bodyyawnormalized), m_LegAnimType(leganimtype), 
		m_bUseAimSequences(useaimsequences), m_bInvertPoseParameters(invertposeparameters) {} 

	// This tells how far the upper body can rotate left and right. If he begins to rotate
	// past this, it'll turn his feet to face his upper body.
	float	m_flMaxBodyYawDegrees;
	bool	m_bBodyYawNormalized;

	// How do the legs animate?
	LegAnimType_t m_LegAnimType;

	// Use aim sequences? (CS hostages don't).
	bool m_bUseAimSequences;

	// Invert pose parameters? (tf2)
	bool m_bInvertPoseParameters;
};


// ------------------------------------------------------------------------------------------------ //
// UnitAnimState declaration.
// ------------------------------------------------------------------------------------------------ //

class UnitAnimState : public UnitBaseAnimState
{
public:
	DECLARE_CLASS_NOBASE( UnitAnimState );

	enum
	{
		TURN_NONE = 0,
		TURN_LEFT,
		TURN_RIGHT
	};
#ifdef ENABLE_PYTHON
						UnitAnimState( boost::python::object outer, UnitAnimConfig &animconfig );
#endif // ENABLE_PYTHON
	virtual ~UnitAnimState();

	// Update() and DoAnimationEvent() together maintain the entire player's animation state.
	//
	// Update() maintains the the lower body animation (the player's m_nSequence)
	// and the upper body overlay based on the player's velocity and look direction.
	//
	// It also modulates these based on events triggered by DoAnimationEvent.
	virtual void Update( float eyeYaw, float eyePitch );

	// This is called by the client when a new player enters the PVS to clear any events
	// the dormant version of the entity may have been playing.
	virtual void ClearAnimationState();

	// This is called every frame to prepare the animation layers to be filled with data
	// since we reconstruct them every frame (in case they get stomped by the networking
	// or anything else).
	virtual void ClearAnimationLayers();

	// The client uses this to figure out what angles to render the entity with (since as the guy turns,
	// it will change his body_yaw pose parameter before changing his rendered angle).
	virtual const QAngle& GetRenderAngles();

// Overrideables.
public:

	virtual bool ShouldUpdateAnimState();

	// This is called near the start of each frame.
	// The base class figures out the main sequence and the aim sequence, and derived
	// classes can overlay whatever other animations they want.
	virtual void ComputeSequences( CStudioHdr *pStudioHdr );

	// This is called when a new model is set. Used for one time setting up of paramters.
	virtual void OnNewModel() {}

	// This is called after a specific sequence is done. Can return a new specific sequence.
	virtual Activity OnEndSpecificActivity( Activity specificactivity ) { return ACT_INVALID; } 

	void SetCustomSpecificActPlaybackRate( float playbackrate );
	float GetCustomSpecificActPlaybackRate() { return m_fSpecMainPlaybackRate; }

	// This is called to figure out what the main activity is. The mod-specific class 
	// overrides this to handle events like jumping, firing, etc.
	virtual Activity CalcMainActivity();

	virtual bool HandleClimbing( Activity &idealActivity );
	virtual bool HandleJumping( Activity &idealActivity );
	virtual bool HandleCrouching( Activity &idealActivity );

	// This is called to calculate the aim layer sequence. It usually figures out the 
	// animation prefixes and suffixes and calls CalcSequenceIndex().
	virtual int CalcAimLayerSequence( float *flCycle, float *flAimSequenceWeight, bool bForceIdle );

	// Misc sequence
	void ComputeMiscSequence();

	virtual bool ShouldResetGroundSpeed( Activity oldActivity, Activity idealActivity );

	// This lets server-controlled idle sequences to play unchanged on the client
	virtual bool ShouldChangeSequences( void ) const;

	// If this returns true, then it will blend the current aim layer sequence with an idle aim layer
	// sequence based on how fast the character is moving, so it doesn't play the upper-body run at
	// full speed if he's moving really slowly.
	//
	// We return false on this for animations that don't have blends, like reloads.
	virtual bool ShouldBlendAimSequenceToIdle();

	// For the body left/right rotation, some models use a pose parameter and some use a bone controller.
	virtual float SetOuterBodyYaw( float flValue );

	// Return true if the player is allowed to move.
	virtual bool CanThePlayerMove();

	// This is called every frame to see what the maximum speed the player can move is.
	// It is used to determine where to put the move_x/move_y pose parameters or to
	// determine the animation playback rate, based on the player's movement speed.
	// The return value from here is interpolated so the playback rate or pose params don't move sharply.
	virtual float GetCurrentMaxGroundSpeed();

	// Display Con_NPrint output about the animation state. This is called if
	// we're on the client and if cl_showanimstate holds the current entity's index.
	void DebugShowAnimStateFull( int iStartLine );

	virtual void DebugShowAnimState( int iStartLine );
	void AnimStatePrintf( int iLine, const char *pMsg, ... );
	void AnimStateLog( const char *pMsg, ... );

	// Calculate the playback rate for movement layer
	virtual float CalcMovementPlaybackRate( bool *bIsMoving );

	// Check if the unit has this activity (translated)
	bool HasActivity( Activity actDesired );

	// Allow inheriting classes to translate their desired activity, while keeping all
	// internal ACT comparisons using the base activity
	virtual Activity TranslateActivity( Activity actDesired );

#ifdef ENABLE_PYTHON
	// Translate activity map
	void SetActivityMap( boost::python::object activitymap );
	bp::object GetActivityMap();
#endif // ENABLE_PYTHON

	// Aim layer sequence
	void SetAimLayerSequence( const char *pAimLayerSequence );
	const char *GetAimLayerSequence();

	// Misc layer
	virtual int GetMiscSequence() { return m_iMiscSequence; }
	virtual float GetMiscCycle() { return m_flMiscCycle; }

	virtual void SetMiscPlaybackRate( float flRate ) { m_fMiscPlaybackRate = flRate; }

public:
	
	//void				GetPoseParameters( CStudioHdr *pStudioHdr, float poseParameter[MAXSTUDIOPOSEPARAM] );

	void				RestartMainSequence();


// Helpers for the derived classes to use.
protected:

	// Sets up the string you specify, looks for that sequence and returns the index. 
	// Complains in the console and returns 0 if it can't find it.
	virtual int CalcSequenceIndex( const char *pBaseName, ... );

	Activity GetCurrentMainSequenceActivity() const;

	void				GetOuterAbsVelocity( Vector& vel ) const;
	float				GetOuterXYSpeed() const;

	// How long has it been since we cleared the animation state?
	float				TimeSinceLastAnimationStateClear() const;

	float				GetEyeYaw() const { return m_flEyeYaw; }

	void UpdateLayerSequenceGeneric( int iLayer, bool &bEnabled, float &flCurCycle,
									int &iSequence, bool bWaitAtEnd,
									float fBlendIn=0.15f, float fBlendOut=0.15f, bool bMoveBlend = false, 
									float fPlaybackRate=1.0f, bool bUpdateCycle = true );

	void				EndSpecificActivity();

protected:
	
	UnitAnimConfig		m_AnimConfig;

protected:
	int					ConvergeAngles( float goal,float maxrate, float maxgap, float dt, float& current );
	virtual void		ComputePoseParam_MoveYaw( CStudioHdr *pStudioHdr );
	virtual void		ComputePoseParam_BodyPitch( CStudioHdr *pStudioHdr );
	virtual void		ComputePoseParam_BodyYaw();
	virtual void		ComputePoseParam_Lean();

	virtual void		ResetGroundSpeed( void );

public:
	bool m_bPlayFallActInAir;

	// List of pose parameters
	// These are set in the OnNewModel function
	// 9 Way
	int m_iMoveX;
	int m_iMoveY;
	bool m_bFlipMoveY;

	// 8 Way
	int m_iMoveYaw;

	// Body
	int m_iBodyYaw;
	int m_iBodyPitch;

	// Lean
	int m_iLeanYaw;
	int m_iLeanPitch;

	// Body yaw turning
	float m_flFeetYawRate;
	float m_flFaceFrontTime;

	// Jumping.
	bool	m_bNewJump;
	bool	m_bJumping;
	float	m_flJumpStartTime;	
	bool	m_bFirstJumpFrame;

	// Misc layer
	bool m_bPlayingMisc;
	float m_flMiscCycle, m_flMiscBlendOut, m_flMiscBlendIn;
	int m_iMiscSequence;
	bool m_bMiscOnlyWhenStill;
	bool m_bMiscNoOverride;
	float m_fMiscPlaybackRate;

	// Combat state.
	bool m_bUseCombatState;
	float m_fCombatStateTime;
	bool m_bCombatStateIfEnemy;

protected:
	// The player's eye yaw and pitch angles.
	float m_flEyeYaw;
	float m_flEyePitch;

	// The following variables are used for tweaking the yaw of the upper body when standing still and
	//  making sure that it smoothly blends in and out once the player starts moving
	// Direction feet were facing when we stopped moving
	float				m_flGoalFeetYaw;

	float				m_flCurrentFeetYaw;
	bool				m_bCurrentFeetYawInitialized;

	float				m_flCurrentTorsoYaw;

	// To check if they are rotating in place
	float				m_flLastYaw;

	// Time when we stopped moving
	float				m_flLastTurnTime;

	// One of the above enums
	int					m_nTurningInPlace;

	QAngle				m_angRender;

	// Specific main sequence variables
	bool				m_bSpecMainUseCustomPlaybackrate;
	float				m_fSpecMainPlaybackRate;

private:

	// Update the prone state machine.
	void		UpdateProneState();

	// Get the string that's appended to animation names for the player's current weapon.
	const char* GetWeaponSuffix();

	Activity			BodyYawTranslateActivity( Activity activity );

	void				SetOuterPoseParameter( int iParam, float flValue );


	void				EstimateYaw();

	void				ComputeMainSequence();
	void				ComputeAimSequence();

	void				ComputePlaybackRate();

	void UpdateInterpolators();
	float GetInterpolatedGroundSpeed();

public:
	// Specific full-body sequence to play
	Activity		m_nSpecificMainActivity;

private:
	string_t m_sAimLayerSequence;
	
	TranslateActivityMap *m_pActivityMap;
#ifdef ENABLE_PYTHON
	boost::python::object m_pyActivityMap; // Keeps m_pActivityMap not NULL
#endif // ENABLE_PYTHON

	float m_flMaxGroundSpeed;

	float m_flLastAnimationStateClearTime;

	// If he's using 8-way blending, then we blend to this idle 
	int m_iCurrent8WayIdleSequence;
	int m_iCurrent8WayCrouchIdleSequence;

	// Last activity we've used on the lower body. Used to determine if animations should restart.
	Activity m_eCurrentMainSequenceActivity;	

	float				m_flGaitYaw;
	float				m_flStoredCycle;

	Vector2D			m_vLastMovePose;

	void UpdateAimSequenceLayers(
		float flCycle,
		int iFirstLayer,
		bool bForceIdle,
		CSequenceTransitioner *pTransitioner,
		float flWeightScale
		);

	void OptimizeLayerWeights( int iFirstLayer, int nLayers );

	// This gives us smooth transitions between aim anim sequences on the client.
	CSequenceTransitioner	m_IdleSequenceTransitioner;
	CSequenceTransitioner	m_SequenceTransitioner;
};

extern float g_flLastBodyPitch, g_flLastBodyYaw, m_flLastMoveYaw;

#ifdef ENABLE_PYTHON
inline bp::object UnitAnimState::GetActivityMap()
{
	return m_pyActivityMap;
}
#endif // ENABLE_PYTHON

inline Activity UnitAnimState::GetCurrentMainSequenceActivity() const
{
	return m_eCurrentMainSequenceActivity;
}

inline bool UnitAnimState::HasActivity( Activity actDesired ) 
{ 
	return SelectWeightedSequence( TranslateActivity( actDesired ) ) > 0; 
}

#endif // UNIT_ANIMSTATE_H
