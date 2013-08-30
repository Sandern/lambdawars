
#include "cbase.h"
#include "deferred/vgui/vgui_deferred.h"

#include "vgui/ISurface.h"
#include "particles/particles.h"


using namespace vgui;

REGISTER_PROJECTABLE_FACTORY( CVGUIParticles, "particle" );

CVGUIParticles::CVGUIParticles()
{
	m_ViewSetup.m_bOrtho = true;
	m_ViewSetup.zNear = 0;
	m_ViewSetup.zFar = 2000;

	m_ViewSetup.angles = vec3_angle;
	m_ViewSetup.origin = Vector( -50, 0, 0 );

	float flOrthoSize = 256;
	m_ViewSetup.m_OrthoTop = -flOrthoSize;
	m_ViewSetup.m_OrthoLeft = -flOrthoSize;
	m_ViewSetup.m_OrthoBottom = flOrthoSize;
	m_ViewSetup.m_OrthoRight = flOrthoSize;
}

CVGUIParticles::~CVGUIParticles()
{
}

void CVGUIParticles::FlushAllParticles()
{
	m_hParticles.PurgeAndDeleteElements();
}

CParticleCollection *CVGUIParticles::AddParticleSystem( const char *pszName, Vector origin, QAngle angles )
{
	CParticleCollection *pInstance = g_pParticleSystemMgr->CreateParticleCollection( pszName );

	if ( pInstance == NULL )
		return NULL;

	Quaternion quat;
	AngleQuaternion( angles, quat );

	pInstance->StartEmission();
	pInstance->SetControlPoint( 0, origin );
	pInstance->SetControlPointOrientation( 0, quat );

	m_hParticles.AddToTail( pInstance );

	return pInstance;
}

void CVGUIParticles::PerformLayout()
{
	BaseClass::PerformLayout();

	SetPaintBackgroundEnabled( false );

	int x,y,w,h;
	GetSize( w, h );
	x = y = 0;
	LocalToScreen( x, y );

	m_ViewSetup.x = x;
	m_ViewSetup.y = y;
	m_ViewSetup.width = w;
	m_ViewSetup.height = h;
}

void CVGUIParticles::Paint()
{
	Frustum frustum;
	render->Push3DView( m_ViewSetup, 0, NULL, frustum );

	CMatRenderContextPtr pRenderContext( materials );

	Vector4D baseDiffuse( 1, 1, 1, 1 );
	FOR_EACH_VEC( m_hParticles, i )
	{
		m_hParticles[ i ]->Render( pRenderContext, baseDiffuse );
	}

	render->PopView( frustum );
}

void CVGUIParticles::OnThink()
{
	FOR_EACH_VEC( m_hParticles, i )
	{
		m_hParticles[ i ]->Simulate( gpGlobals->frametime );

		if ( m_hParticles[ i ]->IsFinished() )
		{
			delete m_hParticles[ i ];
			m_hParticles.Remove( i );
			i--;
		}
	}
}

static Vector GetVectorFromKV( KeyValues *pKV, const char *pszKey, float *pFl3Default )
{
	Vector out;

	const char *pszDefault = "";
	const char *pszVec = pKV->GetString( pszKey, pszDefault );

	if ( pszVec == pszDefault )
		Q_memcpy( out.Base(), pFl3Default, sizeof(float)*3 );
	else
		UTIL_StringToVector( out.Base(), pszVec );

	return out;
}

#define GETVECKV_DEF( targetVector, keyvalues, key, defaultVector ) Q_memcpy(\
	targetVector.Base(),\
	GetVectorFromKV( keyvalues, key, defaultVector.Base() ).Base(),\
	sizeof(float)*3 )
#define GETVECKV( targetVector, keyvalues, key ) GETVECKV_DEF( targetVector, keyvalues, key, targetVector )

void CVGUIParticles::ApplyConfig( KeyValues *pKV )
{
	GETVECKV( m_ViewSetup.origin, pKV, "origin" );
	GETVECKV( m_ViewSetup.angles, pKV, "angles" );

	m_ViewSetup.zNear = pKV->GetFloat( "znear", m_ViewSetup.zNear );
	m_ViewSetup.zFar = pKV->GetFloat( "zfar", m_ViewSetup.zFar );

	float flOrthoSize = pKV->GetFloat( "orthoscale", 256 );
	m_ViewSetup.m_OrthoTop = -flOrthoSize;
	m_ViewSetup.m_OrthoLeft = -flOrthoSize;
	m_ViewSetup.m_OrthoBottom = flOrthoSize;
	m_ViewSetup.m_OrthoRight = flOrthoSize;

	KeyValues *pParticles = pKV->FindKey( "particles" );

	if ( pParticles == NULL )
		return;

	for ( KeyValues *pKVParticles = pParticles->GetFirstTrueSubKey();
		pKVParticles;
		pKVParticles = pKVParticles->GetNextTrueSubKey() )
	{
		Vector pos = vec3_origin;
		QAngle ang = vec3_angle;

		const char *pszParticleName = pKVParticles->GetString( "name" );
		if ( !pszParticleName || !*pszParticleName )
			continue;

		GETVECKV( pos, pKVParticles, "origin" );
		GETVECKV( ang, pKVParticles, "angles" );

		AddParticleSystem( pszParticleName, pos, ang );
	}
}