//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "c_wars_fx.h"
#include "precache_register.h"
#include "bone_setup.h"
#include "util_shared.h"
#include "unit_base_shared.h"

#ifdef DEFERRED_ENABLED
#include "deferred/cdeferred_manager_client.h"
#include "deferred/deferred_shared_common.h"
#endif // DEFERRED_ENABLED

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar max_muzzleflash_dlights( "max_muzzleflash_dlights", "5" );

#ifdef DEFERRED_ENABLED
void DoDeferredMuzzleFlash( const Vector &vOrigin )
{
	if( GetLightingManager()->CountTempLights() > max_muzzleflash_dlights.GetInt() )
		return;

	def_light_temp_t *l = new def_light_temp_t( false, 0.1f );

	l->ang = vec3_angle;
	l->pos = vOrigin;

	l->col_diffuse = Vector( 0.964705882f, 0.82745098f, 0.403921569f ); //GetColor_Diffuse();
	//l->col_ambient = Vector(20, 20, 20); //GetColor_Ambient();

	l->flRadius = 512.0f;
	l->flFalloffPower = 1.0f;

	l->iVisible_Dist = 1024.0f;
	l->iVisible_Range = 1024.0f;
	l->iShadow_Dist = 512.0f;
	l->iShadow_Range = 512.0f;

	l->iFlags >>= DEFLIGHTGLOBAL_FLAGS_MAX_SHARED_BITS;
	l->iFlags <<= DEFLIGHTGLOBAL_FLAGS_MAX_SHARED_BITS;
	l->iFlags |= DEFLIGHT_SHADOW_ENABLED;

	GetLightingManager()->AddTempLight( l );
}
#endif // 0

enum ASW_Bullet_Attribute_Bits
{
	BULLET_ATT_FIRE				=	0x00000001,
	BULLET_ATT_EXPLODE			=	0x00000002,
	BULLET_ATT_CHEMICAL			=	0x00000004,
	BULLET_ATT_ELECTRIC			=	0x00000008,
	BULLET_ATT_FREEZE			=	0x00000010,

	// Highest legal value
	BULLET_ATT_MAX				=	0x00000020,
};

void ASWDoParticleTracer( const char *pTracerEffectName, const Vector &vecStart, const Vector &vecEnd, const Vector &vecColor, int iAttributeEffects = 0 )
{
	VPROF_BUDGET( "ASWDoParticleTracer", VPROF_BUDGETGROUP_PARTICLE_RENDERING );

	// we want to aggregate particles because we are spawning a lot in rapid succession for tracers - this spawns MUCH less systems to spawn them
	// NOTE: you cannot use beams/ropes in any particle system that is aggregated - they don't work properly :(
	CUtlReference<CNewParticleEffect> pTracer = CNewParticleEffect::CreateOrAggregate( NULL, pTracerEffectName, vecStart, NULL );
	if ( pTracer.IsValid() && pTracer->IsValid() )
	{
		pTracer->SetControlPoint( 0, vecStart );
		pTracer->SetControlPoint( 1, vecEnd );
		// the color
		//Vector vecColor = Vector( 1, 1, 1 );
		//if ( bRedTracer )
		//	vecColor = Vector( 1, 0.65, 0.65 );

		pTracer->SetControlPoint( 10, vecColor );
	}

	if ( iAttributeEffects > 0 )
	{
		CUtlReference<CNewParticleEffect> pAttribTracer = CNewParticleEffect::CreateOrAggregate( NULL, "attrib_tracer_fx", vecStart, NULL );
		if ( pAttribTracer.IsValid() && pAttribTracer->IsValid() )
		{
			pAttribTracer->SetControlPoint( 0, vecStart );
			pAttribTracer->SetControlPoint( 1, vecEnd );

			/*
			20.0	freeze
			20.1	fire
			20.2	electric
			21.0	chem
			21.1	explode
			*/

			pAttribTracer->SetControlPoint( 20, Vector( (iAttributeEffects&BULLET_ATT_FREEZE)	? 1.1f : 0, (iAttributeEffects&BULLET_ATT_FIRE)		? 1.1f : 0, (iAttributeEffects&BULLET_ATT_ELECTRIC) ? 1.1f : 0 ) );
			pAttribTracer->SetControlPoint( 21, Vector( (iAttributeEffects&BULLET_ATT_CHEMICAL) ? 1.1f : 0, (iAttributeEffects&BULLET_ATT_EXPLODE)	? 1.1f : 0, 0 ) );
		}
	}
}

static int asw_num_u_tracers = 0;
void ASWUTracer(CBaseCombatCharacter *pUnit, const Vector &vecEnd, const Vector &vecColor, int iAttributeEffects )
{
	MDLCACHE_CRITICAL_SECTION();
	Vector vecStart;
	QAngle vecAngles;

	if ( !pUnit || pUnit->IsDormant() )
		return;

	CBaseCombatWeapon *pWpn = pUnit->GetActiveWeapon();	
	if ( !pWpn || pWpn->IsDormant() )
		return;

	C_BaseAnimating::PushAllowBoneAccess( true, false, "ASWUTracer" );

	pWpn->ProcessMuzzleFlashEvent();

	int nAttachmentIndex = pWpn->LookupAttachment( "muzzle" );
	if ( nAttachmentIndex <= 0 )
		nAttachmentIndex = 1;	// default to the first attachment if it doesn't have a muzzle

	// Get the muzzle origin
	if ( !pWpn->GetAttachment( nAttachmentIndex, vecStart, vecAngles ) )
	{
		return;
	}

#ifdef DEFERRED_ENABLED
	if( GetDeferredManager()->IsDeferredRenderingEnabled() )
		DoDeferredMuzzleFlash( vecStart );
#endif // DEFERRED_ENABLED

	asw_num_u_tracers++;
	//ASWDoParticleTracer( pWpn, vecStart, vecEnd, pWpn->GetMuzzleFlashRed(), iAttributeEffects );
	ASWDoParticleTracer( "tracer_rifle", vecStart, vecEnd, vecColor, false );

	// do a trace to the hit surface for impacts
	trace_t tr;
	Vector diff = vecStart - vecEnd;
	diff.NormalizeInPlace();
	diff *= 6;	// go 6 inches away from surfaces
	CTraceFilterSimple traceFilter(pUnit ,COLLISION_GROUP_NONE);
	UTIL_TraceLine(vecEnd + diff, vecEnd - diff, MASK_SHOT, &traceFilter, &tr);
	// do impact effect
	UTIL_ImpactTrace( &tr, DMG_BULLET );

	C_BaseAnimating::PopBoneAccess( "ASWUTracer" );
}
