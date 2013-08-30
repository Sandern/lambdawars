
#include "cbase.h"
#include "deferred/deferred_shared_common.h"

static CUtlVector< CDeferredLightContainer* >__g_pLightContainerDict;

int GetNumLightContainers()
{
	return __g_pLightContainerDict.Count();
}

CDeferredLightContainer *GetLightContainer( int index )
{
	return __g_pLightContainerDict[ index ];
}

CDeferredLightContainer *FindAvailableContainer()
{
	for ( int i = 0; i < __g_pLightContainerDict.Count(); i++ )
		if ( __g_pLightContainerDict[i]->GetLightsAmount() < DEFLIGHTCONTAINER_MAXLIGHTS )
			return __g_pLightContainerDict[i];

	return NULL;
}

IMPLEMENT_NETWORKCLASS_DT( CDeferredLightContainer, CDeferredLightContainer_DT )

#ifdef GAME_DLL
	SendPropInt( SENDINFO( m_iLightCount ), DEFLIGHTCONTAINER_MAXLIGHT_BITS, SPROP_UNSIGNED ),

	SendPropArray3( SENDINFO_ARRAY3( m_pos ), SendPropVector( SENDINFO_ARRAY( m_pos ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_ang ), SendPropQAngles( SENDINFO_ARRAY( m_ang ) ) ),

	SendPropArray3( SENDINFO_ARRAY3( m_col_diff ), SendPropVector( SENDINFO_ARRAY( m_col_diff ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_col_amb ), SendPropVector( SENDINFO_ARRAY( m_col_amb ) ) ),

	SendPropArray3( SENDINFO_ARRAY3( m_style_amt_speed_smooth ), SendPropVector( SENDINFO_ARRAY( m_style_amt_speed_smooth ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_ranges_vdist_vrange_sdist ), SendPropVector( SENDINFO_ARRAY( m_ranges_vdist_vrange_sdist ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_style_ran_radius_power ), SendPropVector( SENDINFO_ARRAY( m_style_ran_radius_power ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_ranges_srange_cinner_couter ), SendPropVector( SENDINFO_ARRAY( m_ranges_srange_cinner_couter ) ) ),

	SendPropArray3( SENDINFO_ARRAY3( m_type_flags_cookieindex_seed ),
	SendPropInt( SENDINFO_ARRAY( m_type_flags_cookieindex_seed ),
		(DEFLIGHT_SEED_MAX_BITS + MAX_COOKIE_TEXTURES_BITS + DEFLIGHT_FLAGS_MAX_SHARED_BITS + MAX_DEFLIGHTTYPE_BITS), SPROP_UNSIGNED ) ),
#else
	RecvPropInt( RECVINFO( m_iLightCount ) ),

	RecvPropArray3( RECVINFO_ARRAY( m_pos ), RecvPropVector( RECVINFO( m_pos[0] ))),
	RecvPropArray3( RECVINFO_ARRAY( m_ang ), RecvPropQAngles( RECVINFO( m_ang[0] ))),

	RecvPropArray3( RECVINFO_ARRAY( m_col_diff ), RecvPropVector( RECVINFO( m_col_diff[0] ))),
	RecvPropArray3( RECVINFO_ARRAY( m_col_amb ), RecvPropVector( RECVINFO( m_col_amb[0] ))),

	RecvPropArray3( RECVINFO_ARRAY( m_style_amt_speed_smooth ), RecvPropVector( RECVINFO( m_style_amt_speed_smooth[0] ))),
	RecvPropArray3( RECVINFO_ARRAY( m_ranges_vdist_vrange_sdist ), RecvPropVector( RECVINFO( m_ranges_vdist_vrange_sdist[0] ))),
	RecvPropArray3( RECVINFO_ARRAY( m_style_ran_radius_power ), RecvPropVector( RECVINFO( m_style_ran_radius_power[0] ))),
	RecvPropArray3( RECVINFO_ARRAY( m_ranges_srange_cinner_couter ), RecvPropVector( RECVINFO( m_ranges_srange_cinner_couter[0] ))),

	RecvPropArray3( RECVINFO_ARRAY( m_type_flags_cookieindex_seed ), RecvPropInt( RECVINFO( m_type_flags_cookieindex_seed[0] ))),
#endif

END_NETWORK_TABLE();

LINK_ENTITY_TO_CLASS( deferred_light_container, CDeferredLightContainer );

CDeferredLightContainer::CDeferredLightContainer()
{
	__g_pLightContainerDict.AddToTail( this );
#ifdef CLIENT_DLL
	m_iSanityCounter = 0;
	m_hLights.SetGrowSize( 10 );
#endif
}

CDeferredLightContainer::~CDeferredLightContainer()
{
	Assert( __g_pLightContainerDict.HasElement( this ) );
	__g_pLightContainerDict.FindAndRemove( this );

#ifdef CLIENT_DLL
	Assert( m_iSanityCounter == 0 );
	Assert( m_hLights.Count() == 0 );
#endif
}

#ifdef GAME_DLL
void CDeferredLightContainer::Activate()
{
	BaseClass::Activate();

	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );
	AddEffects( EF_NODRAW );
}

int CDeferredLightContainer::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

void CDeferredLightContainer::AddWorldLight( CDeferredLight *l )
{
	int targetIndex = GetLightsAmount();

	Assert( targetIndex < DEFLIGHTCONTAINER_MAXLIGHTS );

	float spotConeInnerDot = SPOT_DEGREE_TO_RAD( l->GetSpotCone_Inner());
	float spotConeOuterDot = SPOT_DEGREE_TO_RAD(l->GetSpotCone_Outer());

	int styleSeed = l->GetStyle_Seed();

	if ( styleSeed < 0 )
		styleSeed = RandomInt( 0, DEFLIGHT_SEED_MAX );

	m_pos.Set( targetIndex, l->GetAbsOrigin() );
	m_ang.Set( targetIndex, l->GetAbsAngles() );

	m_col_diff.Set( targetIndex, l->GetColor_Diffuse() );
	m_col_amb.Set( targetIndex, l->GetColor_Ambient() );

	m_style_amt_speed_smooth.Set( targetIndex,
		Vector( l->GetStyle_Amount(), l->GetStyle_Speed(), l->GetStyle_Smooth() ) );
	m_ranges_vdist_vrange_sdist.Set( targetIndex,
		Vector( l->GetVisible_Distance(), l->GetVisible_FadeRange(), l->GetShadow_Distance() ) );
	m_style_ran_radius_power.Set( targetIndex,
		Vector( l->GetStyle_Random(), l->GetRadius(), l->GetFalloffPower() ) );
	m_ranges_srange_cinner_couter.Set( targetIndex,
		Vector( l->GetShadow_FadeRange(), spotConeInnerDot, spotConeOuterDot ) );

	SetEncodedDataInt( targetIndex,
		styleSeed, l->GetCookieIndex(),
		l->GetLight_Flags(), l->GetLight_Type() );

	m_iLightCount = targetIndex + 1;
}

void CDeferredLightContainer::SetEncodedDataInt( const int index,
		int seed, int cookie, int flags, int lighttype )
{
	Assert( index >= 0 && index < DEFLIGHTCONTAINER_MAXLIGHTS );

	int val = seed;
	val <<= MAX_COOKIE_TEXTURES_BITS;

	val |= cookie;
	val <<= DEFLIGHT_FLAGS_MAX_SHARED_BITS;

	val |= flags;
	val <<= MAX_DEFLIGHTTYPE_BITS;

	val |= lighttype;

	m_type_flags_cookieindex_seed.Set( index, val );
}

#else

void CDeferredLightContainer::PostDataUpdate( DataUpdateType_t type )
{
	BaseClass::PostDataUpdate( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		Assert( m_iSanityCounter == 0 );
		m_iSanityCounter++;

		for ( int i = 0; i < GetLightsAmount(); i++ )
		{
			def_light_t *l = new def_light_t( true );
			m_hLights.AddToTail( l );

			ReadWorldLight( i, *l );

			l->MakeDirtyAll();

			GetLightingManager()->AddLight( l );
		}
	}
}

void CDeferredLightContainer::UpdateOnRemove()
{
	BaseClass::UpdateOnRemove();

	Assert( m_iSanityCounter == 1 );
	m_iSanityCounter--;

	for ( int i = 0; i < m_hLights.Count(); i++ )
		GetLightingManager()->RemoveLight( m_hLights[i] );

	m_hLights.PurgeAndDeleteElements();
}

void CDeferredLightContainer::ReadWorldLight( int index, def_light_t &l )
{
	Assert( index >= 0 && index < GetLightsAmount() );
	Vector tmp;

	l.pos = m_pos.Get( index );
	l.ang = m_ang.Get( index );

	l.col_diffuse = m_col_diff.Get( index );
	l.col_ambient = m_col_amb.Get( index );

	tmp = m_style_amt_speed_smooth.Get( index );
	l.flStyle_Amount = tmp.x;
	l.flStyle_Speed = tmp.y;
	l.flStyle_Smooth = tmp.z;

	tmp = m_ranges_vdist_vrange_sdist.Get( index );
	l.iVisible_Dist = tmp.x;
	l.iVisible_Range = tmp.y;
	l.iShadow_Dist = tmp.z;

	tmp = m_style_ran_radius_power.Get( index );
	l.flStyle_Random = tmp.x;
	l.flRadius = tmp.y;
	l.flFalloffPower = tmp.z;
	
	tmp = m_ranges_srange_cinner_couter.Get( index );
	l.iShadow_Range = tmp.x;
	l.flSpotCone_Inner = tmp.y;
	l.flSpotCone_Outer = tmp.z;

	ReadEncodedDataInt( index, l.iStyleSeed, l.iCookieIndex, l.iFlags, l.iLighttype );
}

#endif

int CDeferredLightContainer::GetLightsAmount()
{
	return m_iLightCount;
}

void CDeferredLightContainer::ReadEncodedDataInt( const int index,
		ushort &seed, uint8 &cookie, uint8 &flags, uint8 &lighttype )
{
	Assert( index >= 0 && index < GetLightsAmount() );

	int val = m_type_flags_cookieindex_seed.Get( index );

	lighttype = val & NETWORK_MASK_LIGHTTYPE;
	val >>= MAX_DEFLIGHTTYPE_BITS;

	flags = val & NETWORK_MASK_FLAGS;
	val >>= DEFLIGHT_FLAGS_MAX_SHARED_BITS;

	cookie = val & NETWORK_MASK_COOKIE;
	val >>= MAX_COOKIE_TEXTURES_BITS;

	seed = val & NETWORK_MASK_SEED;
}