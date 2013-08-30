
#include "cbase.h"
#include "deferred/deferred_shared_common.h"

#include "view_shared.h"
#include "BSPTreeData.h"
#include "CollisionUtils.h"
#include "RayTrace.h"

class CLightLeafEnum : public ISpatialLeafEnumerator
{
public:
	bool EnumerateLeaf( int leaf, int context )
	{
		m_LeafList.AddToTail( leaf );
		return true;
	}

	CUtlVectorFixedGrowable< int, 512 > m_LeafList;
};

BEGIN_DATADESC_NO_BASE( def_light_t )

	DEFINE_KEYFIELD( pos, FIELD_VECTOR, GetLightParamName( LPARAM_ORIGIN ) ),
	DEFINE_KEYFIELD( ang, FIELD_VECTOR, GetLightParamName( LPARAM_ANGLES ) ),

	DEFINE_KEYFIELD( col_diffuse, FIELD_VECTOR, GetLightParamName( LPARAM_DIFFUSE ) ),
	DEFINE_KEYFIELD( col_ambient, FIELD_VECTOR, GetLightParamName( LPARAM_AMBIENT ) ),

	DEFINE_KEYFIELD( flRadius, FIELD_FLOAT, GetLightParamName( LPARAM_RADIUS ) ),
	DEFINE_KEYFIELD( flFalloffPower, FIELD_FLOAT, GetLightParamName( LPARAM_POWER ) ),
	DEFINE_KEYFIELD( flSpotCone_Inner, FIELD_FLOAT, GetLightParamName( LPARAM_SPOTCONE_INNER ) ),
	DEFINE_KEYFIELD( flSpotCone_Outer, FIELD_FLOAT, GetLightParamName( LPARAM_SPOTCONE_OUTER ) ),

	DEFINE_KEYFIELD( iVisible_Dist, FIELD_SHORT, GetLightParamName( LPARAM_VIS_DIST ) ),
	DEFINE_KEYFIELD( iVisible_Range, FIELD_SHORT, GetLightParamName( LPARAM_VIS_RANGE ) ),
	DEFINE_KEYFIELD( iShadow_Dist, FIELD_SHORT, GetLightParamName( LPARAM_SHADOW_DIST ) ),
	DEFINE_KEYFIELD( iShadow_Range, FIELD_SHORT, GetLightParamName( LPARAM_SHADOW_RANGE ) ),

	DEFINE_KEYFIELD( iLighttype, FIELD_CHARACTER, GetLightParamName( LPARAM_LIGHTTYPE ) ),
	DEFINE_KEYFIELD( iFlags, FIELD_CHARACTER, GetLightParamName( LPARAM_SPAWNFLAGS ) ),
	DEFINE_KEYFIELD( iCookieIndex, FIELD_CHARACTER, GetLightParamName( LPARAM_COOKIETEX ) ),

	DEFINE_KEYFIELD( flStyle_Amount, FIELD_FLOAT, GetLightParamName( LPARAM_STYLE_AMT ) ),
	DEFINE_KEYFIELD( flStyle_Speed, FIELD_FLOAT, GetLightParamName( LPARAM_STYLE_SPEED ) ),
	DEFINE_KEYFIELD( flStyle_Smooth, FIELD_FLOAT, GetLightParamName( LPARAM_STYLE_SMOOTH ) ),
	DEFINE_KEYFIELD( flStyle_Random, FIELD_FLOAT, GetLightParamName( LPARAM_STYLE_RANDOM ) ),
	DEFINE_KEYFIELD( iStyleSeed, FIELD_SHORT, GetLightParamName( LPARAM_STYLE_SEED ) ),

#if DEFCFG_ADAPTIVE_VOLUMETRIC_LOD
	DEFINE_KEYFIELD( flVolumeLOD0Dist, FIELD_FLOAT, GetLightParamName( LPARAM_VOLUME_LOD0_DIST ) ),
	DEFINE_KEYFIELD( flVolumeLOD1Dist, FIELD_FLOAT, GetLightParamName( LPARAM_VOLUME_LOD1_DIST ) ),
	DEFINE_KEYFIELD( flVolumeLOD2Dist, FIELD_FLOAT, GetLightParamName( LPARAM_VOLUME_LOD2_DIST ) ),
	DEFINE_KEYFIELD( flVolumeLOD3Dist, FIELD_FLOAT, GetLightParamName( LPARAM_VOLUME_LOD3_DIST ) ),
#endif
#if DEFCFG_CONFIGURABLE_VOLUMETRIC_LOD
	DEFINE_KEYFIELD( iVolumeSamples, FIELD_SHORT, GetLightParamName( LPARAM_VOLUME_SAMPLES ) ),
#endif
END_DATADESC()

IMesh *def_light_t::pMeshUnitSphere = NULL;

def_light_t::def_light_t( bool bWorld )
{
	bWorldLight = bWorld;

	pos.Init();
	ang.Init();

	col_diffuse.Init( 1, 1, 1 );
	col_ambient.Init();

	flRadius = 256.0f;
	flFalloffPower = 2.0f;

	iLighttype = DEFLIGHTTYPE_POINT;
	iFlags = DEFLIGHT_DIRTY_XFORMS | DEFLIGHT_DIRTY_RENDERMESH;
	iCookieIndex = 0;
	iOldCookieIndex = 0;
	pCookie = NULL;

	flSpotCone_Inner = SPOT_DEGREE_TO_RAD( 35 );
	flSpotCone_Outer = SPOT_DEGREE_TO_RAD( 45 );
	flFOV = 90.0f;

	iVisible_Dist = 2048;
	iVisible_Range = 512;
	iShadow_Dist = 1536;
	iShadow_Range = 512;

	iStyleSeed = RandomInt( 0, DEFLIGHT_SEED_MAX );
	flStyle_Amount = 0;
	flStyle_Smooth = 0;
	flStyle_Random = 0;
	flStyle_Speed = 0;
	flLastRandomTime = 0;
	flLastRandomValue = 0;

	iNumLeaves = 0;

#if DEFCFG_ADAPTIVE_VOLUMETRIC_LOD
	flVolumeLOD0Dist = 128;
	flVolumeLOD1Dist = 256;
	flVolumeLOD2Dist = 512;
	flVolumeLOD3Dist = 1024;
#endif

#if DEFCFG_CONFIGURABLE_VOLUMETRIC_LOD
	iVolumeSamples= 50;
#endif

	worldTransform.Identity();

#if DEBUG
	pMesh_Debug = NULL;
	pMesh_Debug_Volumetrics = NULL;
#endif

	pMesh_World = NULL;
	pMesh_Volumetrics = NULL;
	pMesh_VolumPrepass = NULL;
}

def_light_t::~def_light_t()
{
	ClearCookie();

	DestroyMeshes();
}

void def_light_t::DestroyMeshes()
{
	if ( IsPoint() )
		return;

	CMatRenderContextPtr pRenderContext( materials );

	if ( pMesh_World )
		pRenderContext->DestroyStaticMesh( pMesh_World );

	if ( pMesh_Volumetrics )
		pRenderContext->DestroyStaticMesh( pMesh_Volumetrics );

	if ( pMesh_VolumPrepass )
		pRenderContext->DestroyStaticMesh( pMesh_VolumPrepass );

#if DEBUG
	if ( pMesh_Debug )
		pRenderContext->DestroyStaticMesh( pMesh_Debug );

	if ( pMesh_Debug_Volumetrics )
		pRenderContext->DestroyStaticMesh( pMesh_Debug_Volumetrics );
#endif
}

void def_light_t::ShutdownSharedMeshes()
{
	CMatRenderContextPtr pRenderContext( materials );

	if ( pMeshUnitSphere )
		pRenderContext->DestroyStaticMesh( pMeshUnitSphere );
}

def_light_t *def_light_t::AllocateFromKeyValues( KeyValues *pKV )
{
	def_light_t *pRet = new def_light_t();

	pRet->ApplyKeyValueProperties( pKV );

	return pRet;
}

KeyValues *def_light_t::AllocateAsKeyValues()
{
	typedescription_t *dt = GetDataDescMap()->dataDesc;
	int iNumFields = GetDataDescMap()->dataNumFields;

	KeyValues *pRet = new KeyValues( "deflight" );

	for ( int iField = 0; iField < iNumFields; iField++ )
	{
		typedescription_t &pField = dt[ iField ];
		const char *pszFieldName = pField.externalName;

		if ( !Q_stricmp( pszFieldName, GetLightParamName( LPARAM_DIFFUSE ) ) ||
			!Q_stricmp( pszFieldName, GetLightParamName( LPARAM_AMBIENT ) ) )
		{
			Vector col;
			Q_memcpy( col.Base(), (void*)((int)this + pField.fieldOffset), pField.fieldSizeInBytes );

			char tmp[256] = {0};
			vecToStringCol( col, tmp, sizeof( tmp ) );

			pRet->SetString( pszFieldName, tmp );
		}
		else if ( !Q_stricmp( pszFieldName, GetLightParamName( LPARAM_COOKIETEX ) ) )
		{
			int iCookieIndex = 0;
			Q_memcpy( &iCookieIndex, (void*)((int)this + (int)pField.fieldOffset), pField.fieldSizeInBytes );

			const char *pszCookieName = g_pStringTable_LightCookies->GetString( iCookieIndex );

			if ( pszCookieName != NULL && *pszCookieName )
			{
				pRet->SetString( pszFieldName, pszCookieName );
			}
		}
		else if ( !Q_stricmp( pszFieldName, GetLightParamName( LPARAM_ORIGIN ) ) ||
			!Q_stricmp( pszFieldName, GetLightParamName( LPARAM_ANGLES ) ) )
		{
			Vector pos;
			Q_memcpy( pos.Base(), (void*)((int)this + (int)pField.fieldOffset), pField.fieldSizeInBytes );

			pRet->SetString( pszFieldName, VarArgs( "%.2f %.2f %.2f", XYZ( pos ) ) );
		}
		else
		{
			switch ( pField.fieldType )
			{
			default:
					Assert( 0 );
				break;
			case FIELD_FLOAT:
				{
					float fl1 = 0;
					Q_memcpy( &fl1, (void*)((int)this + (int)pField.fieldOffset), pField.fieldSizeInBytes );

					if ( !Q_stricmp( pszFieldName, GetLightParamName( LPARAM_SPOTCONE_INNER ) ) ||
						!Q_stricmp( pszFieldName, GetLightParamName( LPARAM_SPOTCONE_OUTER ) ) )
						fl1 = SPOT_RAD_TO_DEGREE( fl1 );

					pRet->SetFloat( pszFieldName, fl1 );
				}
				break;
			case FIELD_SHORT:
			case FIELD_CHARACTER:
				{
					int it1 = 0;
					Q_memcpy( &it1, (void*)((int)this + (int)pField.fieldOffset), pField.fieldSizeInBytes );
					pRet->SetInt( pszFieldName, it1 );
				}
				break;
			}
		}
	}

	return pRet;
}

void def_light_t::ApplyKeyValueProperties( KeyValues *pKV )
{
	typedescription_t *dt = GetDataDescMap()->dataDesc;
	int iNumFields = GetDataDescMap()->dataNumFields;

	for ( KeyValues *pValue = pKV->GetFirstValue(); pValue; pValue = pValue->GetNextValue() )
	{
		const char *pszKeyName = pValue->GetName();

		if ( !pszKeyName || !*pszKeyName )
			continue;

		for ( int iField = 0; iField < iNumFields; iField++ )
		{
			typedescription_t &pField = dt[ iField ];

			if ( !Q_stricmp( pField.externalName, pszKeyName ) )
			{
				if ( !Q_stricmp( pszKeyName, GetLightParamName( LPARAM_DIFFUSE ) ) ||
					!Q_stricmp( pszKeyName, GetLightParamName( LPARAM_AMBIENT ) ) )
				{
					Vector col = stringColToVec( pValue->GetString() );
					Q_memcpy( (void*)((int)this + (int)pField.fieldOffset), col.Base(), pField.fieldSizeInBytes );
				}
				else if ( !Q_stricmp( pszKeyName, GetLightParamName( LPARAM_COOKIETEX ) ) )
				{
					const char *pszTextureName = pValue->GetString();
					IDefCookie *pCookieValidate = CreateCookieInstance( pszTextureName );

					if ( pCookieValidate != NULL )
					{
						delete pCookieValidate;

						int iAddedCookie = g_pStringTable_LightCookies->AddString( true, pszTextureName );
						Q_memcpy( (void*)((int)this + (int)pField.fieldOffset), &iAddedCookie, pField.fieldSizeInBytes );
					}
				}
				else if ( !Q_stricmp( pszKeyName, GetLightParamName( LPARAM_ORIGIN ) ) ||
					!Q_stricmp( pszKeyName, GetLightParamName( LPARAM_ANGLES ) ) )
				{
					float f[3] = { 0 };
					UTIL_StringToFloatArray( f, 3, pValue->GetString() );
					Q_memcpy( (void*)((int)this + (int)pField.fieldOffset), f, pField.fieldSizeInBytes );
				}
				else
				{
					switch ( pField.fieldType )
					{
					default:
							Assert( 0 );
						break;
					case FIELD_FLOAT:
						{
							float fl1 = pValue->GetFloat();

							if ( !Q_stricmp( pszKeyName, GetLightParamName( LPARAM_SPOTCONE_INNER ) ) ||
								!Q_stricmp( pszKeyName, GetLightParamName( LPARAM_SPOTCONE_OUTER ) ) )
								fl1 = SPOT_DEGREE_TO_RAD( fl1 );

							Q_memcpy( (void*)((int)this + (int)pField.fieldOffset), &fl1, pField.fieldSizeInBytes );
						}
						break;
					case FIELD_SHORT:
					case FIELD_CHARACTER:
						{
							int it1 = pValue->GetInt();
							Q_memcpy( (void*)((int)this + (int)pField.fieldOffset), &it1, pField.fieldSizeInBytes );
						}
						break;
					}
				}

				break;
			}
		}
	}

	MakeDirtyAll();
}

void def_light_t::UpdateCookieTexture()
{
	if ( !HasCookie() )
		return;

	if ( iOldCookieIndex == iCookieIndex &&
		pCookie != NULL && pCookie->IsValid() )
		return;

	const char *pszCookieName = g_pStringTable_LightCookies->GetString( iCookieIndex );

	if ( pszCookieName == NULL || *pszCookieName == '\0' )
		return;

	delete pCookie;
	pCookie = NULL;

	pCookie = CreateCookieInstance( pszCookieName );

	iOldCookieIndex = iCookieIndex;
}

bool def_light_t::IsCookieReady()
{
	return pCookie != NULL && pCookie->IsValid();
}

ITexture *def_light_t::GetCookieForDraw( const int iTargetIndex )
{
	Assert( pCookie != NULL );

	pCookie->PreRender( iTargetIndex );
	return pCookie->GetCookieTarget( iTargetIndex );
}

void def_light_t::SetCookie( IDefCookie *pCookie )
{
	ClearCookie();
	this->pCookie = pCookie;
}

void def_light_t::ClearCookie()
{
	delete pCookie;
	pCookie = NULL;
}

IDefCookie *def_light_t::CreateCookieInstance( const char *pszCookieName )
{
	CVGUIProjectable *pVProj = CProjectableFactory::AllocateProjectableByScript( pszCookieName );
	if ( pVProj != NULL )
		return new CDefCookieProjectable( pVProj );

	ITexture *pTex = materials->FindTexture( pszCookieName, TEXTURE_GROUP_OTHER );
	if ( !IsErrorTexture( pTex ) )
		return new CDefCookieTexture( pTex );

	return NULL;
}

void def_light_t::UpdateMatrix()
{
	Assert( iLighttype == DEFLIGHTTYPE_SPOT );

	float aCOS = acos( flSpotCone_Outer );
	flFOV = RAD2DEG( aCOS ) * 2.0f;
	//flConeRadius = tan( aCOS ) * flRadius;

	VMatrix matPerspective, matView, matViewProj;

	matView.Identity();
	matView.SetupMatrixOrgAngles( vec3_origin, ang );
	MatrixSourceToDeviceSpace( matView );
	matView = matView.Transpose3x3();

	Vector viewPosition;
	Vector3DMultiply( matView, pos, viewPosition );
	matView.SetTranslation( -viewPosition );

	MatrixBuildPerspectiveX( matPerspective, flFOV, 1, DEFLIGHT_SPOT_ZNEAR, flRadius );
	MatrixMultiply( matPerspective, matView, matViewProj );

	MatrixInverseGeneral( matViewProj, spotMVPInv );
	MatrixInverseGeneral( matPerspective, spotVPInv );

	VMatrix screenToTexture;
	MatrixBuildScale( screenToTexture, 0.5f, -0.5f, 1.0f );
	screenToTexture[0][3] = screenToTexture[1][3] = 0.5f;
	MatrixMultiply( screenToTexture, matViewProj, spotWorldToTex );

	spotWorldToTex = spotWorldToTex.Transpose();

	UpdateFrustum();
}

void def_light_t::UpdateFrustum()
{
	GeneratePerspectiveFrustum( pos, ang, DEFLIGHT_SPOT_ZNEAR, flRadius, flFOV, 1, spotFrustum );
}

void def_light_t::UpdateXForms()
{
	normalizeAngles( ang );

	Vector fwd;
	AngleVectors( ang, &fwd );
	backDir = -fwd;

	flMaxDistSqr = iVisible_Dist + iVisible_Range;
	flMaxDistSqr *= flMaxDistSqr;

	trace_t tr;

#define __ND0 1.0f
#define __ND1 0.57735f

	switch ( iLighttype )
	{
	default:
		Assert( 0 );
	case DEFLIGHTTYPE_POINT:
		{
			Vector points[14];
			const int numPoints = ARRAYSIZE( points );

			const Vector _dirs[numPoints] = {
				Vector( __ND0, __ND0, __ND0 ),
				Vector( -__ND0, __ND0, __ND0 ),
				Vector( __ND0, -__ND0, __ND0 ),
				Vector( __ND0, __ND0, -__ND0 ),

				Vector( -__ND0, -__ND0, __ND0 ),
				Vector( __ND0, -__ND0, -__ND0 ),
				Vector( -__ND0, __ND0, -__ND0 ),
				Vector( -__ND0, -__ND0, -__ND0 ),

				Vector( __ND0, 0, 0 ),
				Vector( -__ND0, 0, 0 ),
				Vector( 0, __ND0, 0 ),
				Vector( 0, -__ND0, 0 ),
				Vector( 0, 0, __ND0 ),
				Vector( 0, 0, -__ND0 ),
			};

			for ( int i = 0; i < numPoints; i++ )
				points[i] =_dirs[i] * flRadius + pos;

			Vector list[numPoints];
			CTraceFilterWorldOnly filter;
			for ( int i = 0; i < numPoints; i++ )
			{
				UTIL_TraceLine( pos, points[i], MASK_SOLID, &filter, &tr );
				list[ i ] = tr.endpos;
			}

			CalcBoundaries( list, numPoints, bounds_min, bounds_max );

			bounds_min_naive = pos - _dirs[0] * flRadius;
			bounds_max_naive = pos + _dirs[0] * flRadius;

			boundsCenter = pos;
		}
		break;
	case DEFLIGHTTYPE_SPOT:
		{
			Vector points[5];
			const int numPoints = ARRAYSIZE( points );
#if DEFCFG_USE_SSE
			static bool bSIMDDataInitialised = false;
			static fltx4 _normPos[4];
			
			if( !bSIMDDataInitialised )
			{
				_normPos[0] = LoadOneSIMD();

				const float pNormPos1[4] = { -1, 1, 1, 1 },
					pNormPos2[4] = { -1, -1, 1, 1 },
					pNormPos3[4] = { 1, -1, 1, 1 };

				_normPos[1] = _mm_loadu_ps( pNormPos1 );
				_normPos[2] = _mm_loadu_ps( pNormPos2 );
				_normPos[3] = _mm_loadu_ps( pNormPos3 );

				bSIMDDataInitialised = true;
			}

			fltx4 _spotMVPInvSSE[4];

			_spotMVPInvSSE[0] =	_mm_loadu_ps( spotMVPInv[0] );
			_spotMVPInvSSE[1] =	_mm_loadu_ps( spotMVPInv[1] );
			_spotMVPInvSSE[2] =	_mm_loadu_ps( spotMVPInv[2] );
			_spotMVPInvSSE[3] =	_mm_loadu_ps( spotMVPInv[3] );

			TransposeSIMD
				( 
					_spotMVPInvSSE[0],
					_spotMVPInvSSE[1],
					_spotMVPInvSSE[2],
					_spotMVPInvSSE[3] 
				);

			for( int i = 0; i < 4; i++ )
			{
				fltx4 pointx4 = FourDotProducts( _spotMVPInvSSE, _normPos[i] );

				float _w = SubFloat( pointx4, 3 );
				if( _w != 0 )
				{
					_w = 1.0f / _w;
				}

				pointx4 = MulSIMD( pointx4, ReplicateX4( _w ) );

				Q_memcpy( points[i].Base(), &SubFloat( pointx4, 0 ), 3 * sizeof(float) );
			}
#else
			static const Vector _normPos[4] = {
				Vector( 1, 1, 1 ),
				Vector( -1, 1, 1 ),
				Vector( -1, -1, 1 ),
				Vector( 1, -1, 1 ),
			};

			points[4] = pos;

			for ( int i = 0; i < 4; i++ )
				Vector3DMultiplyPositionProjective( spotMVPInv, _normPos[i], points[i] );
#endif
			points[4] = pos;

			Vector list[6];
			Q_memcpy( list, points, sizeof( Vector ) * 4 );
			list[4] = pos + fwd * flRadius;
			list[5] = pos;

			for ( int i = 0; i < 5; i++ )
			{
				RayTracingEnvironment environment;
				UTIL_TraceLine( pos, list[i], MASK_SOLID, NULL, COLLISION_GROUP_DEBRIS, &tr );
				list[ i ] = tr.endpos;
			}

			CalcBoundaries( list, 6, bounds_min, bounds_max );

			CalcBoundaries( points, numPoints, bounds_min_naive, bounds_max_naive );

			boundsCenter = bounds_min_naive + ( bounds_max_naive - bounds_min_naive ) * 0.5f;
		}
		break;
	}

	if ( ( bounds_max - bounds_min ).LengthSqr() < 1 )
	{
		bounds_max = pos + Vector( __ND1, __ND1, __ND1 );
		bounds_min = pos - Vector( __ND1, __ND1, __ND1 );
	}

	QAngle worldAng = ang;
	if ( IsPoint() )
		worldAng.Init();

	if ( iLighttype == DEFLIGHTTYPE_POINT )
	{
		VMatrix tmp;
		tmp.SetupMatrixOrgAngles( vec3_origin, worldAng );

		VMatrix scale;
		scale.Identity();
		MatrixBuildScale( scale, flRadius, flRadius, flRadius );
		MatrixMultiply( tmp, scale, worldTransform );

		worldTransform.SetTranslation( pos );
	}
	else
		worldTransform.SetupMatrixOrgAngles( pos, worldAng );

	CLightLeafEnum leaves;
	ISpatialQuery* pQuery = engine->GetBSPTreeQuery();
	pQuery->EnumerateLeavesInBox( bounds_min, bounds_max, &leaves, 0 );

	AssertMsgOnce( leaves.m_LeafList.Count() <= DEFLIGHT_MAX_LEAVES, "You sure have got a huge light there, or crappy leaves." );

	iNumLeaves = MIN( DEFLIGHT_MAX_LEAVES, leaves.m_LeafList.Count() );
	Q_memcpy( iLeaveIDs, leaves.m_LeafList.Base(), sizeof(int) * iNumLeaves );
}

void def_light_t::UpdateRenderMesh()
{
	if ( iLighttype == DEFLIGHTTYPE_POINT && pMesh_World != NULL )
	{
		Assert( pMesh_Debug );
		Assert( pMeshUnitSphere );
		Assert( pMesh_World == pMeshUnitSphere );
		return;
	}

	CMatRenderContextPtr pRenderContext( materials );

#if DEBUG
	if ( pMesh_Debug != NULL )
		pRenderContext->DestroyStaticMesh( pMesh_Debug );

	pMesh_Debug = pRenderContext->CreateStaticMesh( VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TEXCOORD_SIZE( 0, 2 ),
		TEXTURE_GROUP_OTHER,
		GetDeferredManager()->GetDeferredMaterial( DEF_MAT_WIREFRAME_DEBUG ) );

	switch ( iLighttype )
	{
	default:
		Assert( 0 );
		break;
	case DEFLIGHTTYPE_POINT:
			BuildSphere( &pMesh_Debug );
		break;
	case DEFLIGHTTYPE_SPOT:
			BuildCone( pMesh_Debug );
		break;
	}
#endif

	if ( pMesh_World != NULL )
		pRenderContext->DestroyStaticMesh( pMesh_World );

	IMaterial *pWorldMaterial = NULL;

	switch ( iLighttype )
	{
	default:
		Assert( 0 );
		break;
	case DEFLIGHTTYPE_POINT:
			pWorldMaterial = GetDeferredManager()->GetDeferredMaterial( DEF_MAT_LIGHT_POINT_WORLD );
		break;
	case DEFLIGHTTYPE_SPOT:
			pWorldMaterial = GetDeferredManager()->GetDeferredMaterial( DEF_MAT_LIGHT_SPOT_WORLD );
		break;
	}

	if ( pWorldMaterial == NULL )
		return;

	pMesh_World = pRenderContext->CreateStaticMesh( VERTEX_POSITION | VERTEX_TEXCOORD_SIZE( 0, 2 ),
		TEXTURE_GROUP_OTHER, pWorldMaterial );

	switch ( iLighttype )
	{
	default:
		Assert( 0 );
		break;
	case DEFLIGHTTYPE_POINT:
			BuildSphere( &pMesh_World );
		break;
	case DEFLIGHTTYPE_SPOT:
			BuildCone( pMesh_World );
		break;
	}
}

void def_light_t::UpdateVolumetrics()
{
	if ( iLighttype == DEFLIGHTTYPE_POINT && pMesh_Volumetrics != NULL )
	{
		Assert( pMesh_Debug_Volumetrics );
		Assert( pMeshUnitSphere );
		Assert( pMesh_Volumetrics == pMeshUnitSphere );
		return;
	}

	CMatRenderContextPtr pRenderContext( materials );

#if DEBUG
	if ( pMesh_Debug_Volumetrics != NULL )
		pRenderContext->DestroyStaticMesh( pMesh_Debug_Volumetrics );
	pMesh_Debug_Volumetrics = NULL;

	if ( HasVolumetrics() )
	{
		pMesh_Debug_Volumetrics = pRenderContext->CreateStaticMesh( VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TEXCOORD_SIZE( 0, 2 ),
			TEXTURE_GROUP_OTHER,
			GetDeferredManager()->GetDeferredMaterial( DEF_MAT_WIREFRAME_DEBUG ) );

		switch ( iLighttype )
		{
			default:
			Assert( 0 );
			break;
		case DEFLIGHTTYPE_POINT:
				BuildSphere( &pMesh_Debug_Volumetrics );
			break;
		case DEFLIGHTTYPE_SPOT:
				BuildCone( pMesh_Debug_Volumetrics );
			break;
		}
	}
#endif
	
	if ( pMesh_Volumetrics != NULL )
		pRenderContext->DestroyStaticMesh( pMesh_Volumetrics );
	pMesh_Volumetrics = NULL;

	if ( pMesh_VolumPrepass != NULL )
		pRenderContext->DestroyStaticMesh( pMesh_VolumPrepass );
	pMesh_VolumPrepass = NULL;

	if ( !HasVolumetrics() )
		return;

	switch ( iLighttype )
	{
	default:
		Assert( 0 );
		break;
	case DEFLIGHTTYPE_POINT:
		{
			IMaterial *pVolumeMaterial = GetDeferredManager()->GetDeferredMaterial( DEF_MAT_LIGHT_VOLUME_POINT_WORLD );

			if ( pVolumeMaterial != NULL )
			{
				pMesh_Volumetrics = pRenderContext->CreateStaticMesh( VERTEX_POSITION | VERTEX_TEXCOORD_SIZE( 0, 2 ),
					TEXTURE_GROUP_OTHER, pVolumeMaterial );
				BuildSphere( &pMesh_Volumetrics );
			}
		}
		break;
	case DEFLIGHTTYPE_SPOT:
		{
			IMaterial *pVolumeMaterial = GetDeferredManager()->GetDeferredMaterial( DEF_MAT_LIGHT_VOLUME_SPOT_WORLD );
			IMaterial *pVolumePrepassMaterial = GetDeferredManager()->GetDeferredMaterial( DEF_MAT_LIGHT_VOLUME_PREPASS );

			if ( pVolumeMaterial != NULL && pVolumePrepassMaterial != NULL )
			{
				pMesh_Volumetrics = pRenderContext->CreateStaticMesh( VERTEX_POSITION | VERTEX_TEXCOORD_SIZE( 0, 2 ),
					TEXTURE_GROUP_OTHER, pVolumeMaterial );
				BuildCone( pMesh_Volumetrics );

				pMesh_VolumPrepass = pRenderContext->CreateStaticMesh( VERTEX_POSITION | VERTEX_TEXCOORD_SIZE( 0, 2 ),
					TEXTURE_GROUP_OTHER, pVolumePrepassMaterial );
				BuildConeFlipped( pMesh_VolumPrepass );
			}
		}
		break;
	}
}


void def_light_t::BuildBox( IMesh *pMesh )
{
	CMeshBuilder meshBuilder;

	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 6 );

	meshBuilder.Position3f( -flRadius, -flRadius, -flRadius );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f( flRadius, -flRadius, -flRadius );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f( flRadius, flRadius, -flRadius );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f( -flRadius, flRadius, -flRadius );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( flRadius, flRadius, flRadius );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f( flRadius, flRadius, -flRadius );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f( flRadius, -flRadius, -flRadius );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f( flRadius, -flRadius, flRadius );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( -flRadius, flRadius, flRadius );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f( -flRadius, -flRadius, flRadius );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f( -flRadius, -flRadius, -flRadius );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f( -flRadius, flRadius, -flRadius );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( flRadius, flRadius, flRadius );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f( -flRadius, flRadius, flRadius );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f( -flRadius, flRadius, -flRadius );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f( flRadius, flRadius, -flRadius );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( flRadius, -flRadius, flRadius );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f( flRadius, -flRadius, -flRadius );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f( -flRadius, -flRadius, -flRadius );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f( -flRadius, -flRadius, flRadius );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( flRadius, flRadius, flRadius );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f( flRadius, -flRadius, flRadius );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f( -flRadius, -flRadius, flRadius );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3f( -flRadius, flRadius, flRadius );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
}

void def_light_t::BuildSphere( IMesh **pMesh )
{
	if ( pMeshUnitSphere )
	{
		*pMesh = pMeshUnitSphere;
		return;
	}

	const float flRadius = 1.05f; // overlap member var
	const int iNumLa = 10;
	const int iNumLo = 11;

	const int iNumPoints = iNumLa * iNumLo;
	Vector cachedPoints[ iNumPoints ];

	for ( int iLa = 0; iLa < iNumLa; iLa++ )
	for ( int iLo = 0; iLo < iNumLo; iLo++ )
	{
		const int iPoint = iLa * iNumLo + iLo;
		const float flLa = iLa / (float)( iNumLa - 1.0f );
		const float flLo = iLo / (float)iNumLo;

		Assert( iPoint >= 0 && iPoint < iNumPoints );

		cachedPoints[ iPoint ].Init( sin( M_PI * flLa ) * cos( 2.0f * M_PI * flLo ),
			sin( M_PI * flLa ) * sin( 2.0f * M_PI * flLo ),
			cos( M_PI * flLa ) );

		cachedPoints[ iPoint ] *= flRadius;
	}

	const int iNumVerts = iNumLo * 2 * ( iNumLa - 1 ) + ( iNumLo - 1 ) * 2 + 1;

	CMeshBuilder meshbuilder;
	meshbuilder.Begin( *pMesh, MATERIAL_TRIANGLE_STRIP, iNumVerts );

	for ( int iLo = 0; iLo < iNumLo; iLo++ )
	for ( int iLa = 0; iLa < iNumLa - 1; iLa++ )
	{
		const bool bLimitLo = iLo == iNumLo - 1;
		const bool bLimitLa = iLa == iNumLa - 1;

		int iPoint0 = iLa * iNumLo + iLo;
		int iPoint2 = ( bLimitLa ? 0 : ( iLa + 1 ) * iNumLo ) + iLo;
		int iPoint3 = ( iLa + 1 ) * iNumLo + ( bLimitLo ? 0 : iLo + 1 );

		Assert( iPoint0 >= 0 && iPoint0 < iNumPoints );
		Assert( iPoint2 >= 0 && iPoint2 < iNumPoints );

		Vector p;

		if ( iLa == 0 )
		{
			Assert( iPoint3 >= 0 && iPoint3 < iNumPoints );

			if ( iLo != 0 )
			{
				// degenerate
				meshbuilder.Position3fv( cachedPoints[ iPoint0 ].Base() );
				meshbuilder.AdvanceVertex();
			}

			// top triangle
			meshbuilder.Position3fv( cachedPoints[ iPoint0 ].Base() );
			meshbuilder.AdvanceVertex();

			meshbuilder.Position3fv( cachedPoints[ iPoint3 ].Base() );
			meshbuilder.AdvanceVertex();

			meshbuilder.Position3fv( cachedPoints[ iPoint2 ].Base() );
			meshbuilder.AdvanceVertex();
		}
		else if ( iLa == ( iNumLa - 2 ) )
		{
			// center vert
			meshbuilder.Position3fv( cachedPoints[ iPoint2 ].Base() );
			meshbuilder.AdvanceVertex();

			// degenerate
			meshbuilder.Position3fv( cachedPoints[ iPoint2 ].Base() );
			meshbuilder.AdvanceVertex();
		}
		else
		{
			Assert( iPoint3 >= 0 && iPoint3 < iNumPoints );

			// lower two points per quad
			meshbuilder.Position3fv( cachedPoints[ iPoint3 ].Base() );
			meshbuilder.AdvanceVertex();

			meshbuilder.Position3fv( cachedPoints[ iPoint2 ].Base() );
			meshbuilder.AdvanceVertex();
		}
	}

	meshbuilder.End();

	pMeshUnitSphere = *pMesh;
}

void def_light_t::BuildCone( IMesh *pMesh )
{
	CMeshBuilder meshBuilder;

	static const Vector _normPos[4] = {
		Vector( 1, 1, 1 ),
		Vector( -1, 1, 1 ),
		Vector( -1, -1, 1 ),
		Vector( 1, -1, 1 ),
	};

	Vector world[4];

	for ( int i = 0; i < 4; i++ )
	{
		Vector3DMultiplyPositionProjective( spotVPInv, _normPos[i], world[i] );

		VectorDeviceToSourceSpace( world[i] );
	}

	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, 6 );

	meshBuilder.Position3fv( world[0].Base() );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3fv( world[1].Base() );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3fv( world[2].Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv( world[0].Base() );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3fv( world[2].Base() );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3fv( world[3].Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv( vec3_origin.Base() );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3fv( world[1].Base() );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3fv( world[0].Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv( vec3_origin.Base() );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3fv( world[3].Base() );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3fv( world[2].Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv( vec3_origin.Base() );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3fv( world[2].Base() );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3fv( world[1].Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv( vec3_origin.Base() );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3fv( world[0].Base() );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3fv( world[3].Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
}

void def_light_t::BuildConeFlipped( IMesh *pMesh )
{
	CMeshBuilder meshBuilder;

	// same as worldmesh, just flipped
	// flipping culling the shader seems broken, don't wanna even try flipping it from the client.
	static const Vector _normPos[4] = {
		Vector( 1, 1, 1 ),
		Vector( -1, 1, 1 ),
		Vector( -1, -1, 1 ),
		Vector( 1, -1, 1 ),
	};

	Vector world[4];

	for ( int i = 0; i < 4; i++ )
	{
		Vector3DMultiplyPositionProjective( spotVPInv, _normPos[i], world[i] );

		VectorDeviceToSourceSpace( world[i] );
	}

	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, 6 );

	meshBuilder.Position3fv( world[2].Base() );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3fv( world[1].Base() );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3fv( world[0].Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv( world[3].Base() );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3fv( world[2].Base() );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3fv( world[0].Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv( world[0].Base() );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3fv( world[1].Base() );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3fv( vec3_origin.Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv( world[2].Base() );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3fv( world[3].Base() );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3fv( vec3_origin.Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv( world[1].Base() );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3fv( world[2].Base() );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3fv( vec3_origin.Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3fv( world[3].Base() );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3fv( world[0].Base() );
	meshBuilder.AdvanceVertex();
	meshBuilder.Position3fv( vec3_origin.Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
}
