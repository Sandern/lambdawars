//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: Calculates density values around the entity.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "density_weight_map.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define GRIDHSIZE 1.0f // 2.0f // Multiplied by sigma, defines the size of the computed density field
#define MAP_TILE_SIZE 8.0f // 4.0f // Tilesize of the above grid

#ifndef CLIENT_DLL
void OnDensityConVarChanged( IConVar *var, const char *pOldValue, float flOldValue )
{
	CBaseEntity *pEnt = gEntList.FirstEnt();
	while( pEnt )
	{
		const Vector& vMins = pEnt->CollisionProp()->OBBMins();
		const Vector& vMaxs = pEnt->CollisionProp()->OBBMaxs();
		pEnt->DensityMap()->RecalculateWeights(vMins, vMaxs);

		//pEnt->DensityMap()->OnCollisionSizeChanged();
		pEnt = gEntList.NextEnt( pEnt );
	}
}
#endif // CLIENT_DLL

#ifndef CLIENT_DLL
	ConVar unit_density_tile_res("unit_density_tile_res", "8", FCVAR_REPLICATED, "Change density tile res", OnDensityConVarChanged);

	ConVar unit_density_eclipse_tile_res("unit_density_eclipse_tile_res", "32", FCVAR_REPLICATED, "Change eclipse density tile res", OnDensityConVarChanged);
	ConVar unit_density_eclipse_inside_scale("unit_density_eclipse_inside_scale", "5", FCVAR_REPLICATED, "Change sigma", OnDensityConVarChanged);
	ConVar unit_density_eclipse_sigma_scale("unit_density_eclipse_sigma_scale", "0.3", FCVAR_REPLICATED, "Change sigma", OnDensityConVarChanged);
#else
	ConVar unit_density_tile_res("unit_density_tile_res", "8", FCVAR_REPLICATED, "Change density tile res");

	ConVar unit_density_eclipse_tile_res("unit_density_eclipse_tile_res", "32", FCVAR_REPLICATED, "Change eclipse density tile res");
	ConVar unit_density_eclipse_inside_scale("unit_density_eclipse_inside_scale", "5", FCVAR_REPLICATED, "Change sigma");
	ConVar unit_density_eclipse_sigma_scale("unit_density_eclipse_sigma_scale", "0.3", FCVAR_REPLICATED, "Change sigma");
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
DensityWeightsMap::DensityWeightsMap() 
		: m_pOuter(NULL), m_pWeights(NULL), m_iSizeX(0), m_iSizeY(0), m_iType(DENSITY_NONE)
{
	m_vMins = vec3_origin;
	m_vMaxs = vec3_origin;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
DensityWeightsMap::~DensityWeightsMap()
{
	Destroy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DensityWeightsMap::Init( CBaseEntity *pOuter )
{
	m_pOuter = pOuter;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DensityWeightsMap::Destroy()
{
	int i;
	if( m_pWeights )
	{
		for(i=0; i<m_iSizeX; i++)
			free(m_pWeights[i]);
		free(m_pWeights);
		m_pWeights = NULL;
		m_iSizeX = m_iSizeY = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DensityWeightsMap::SetType( int iType )
{
	if( m_iType == iType )
		return;
	m_iType = iType;
	RecalculateWeights(m_vMins, m_vMaxs);
}

//-----------------------------------------------------------------------------
// Purpose: Recalculate weights
//-----------------------------------------------------------------------------
void DensityWeightsMap::OnCollisionSizeChanged()
{
	const Vector& vMins = m_pOuter->CollisionProp()->OBBMins();
	const Vector& vMaxs = m_pOuter->CollisionProp()->OBBMaxs();
	if( vMins != m_vMins || vMaxs != m_vMaxs )
		RecalculateWeights(vMins, vMaxs);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#define USELERP
float DensityWeightsMap::Get( const Vector &vPos )
{
	Vector vTranslated;
	

#ifdef USELERP
	float x, y;
	int xlow, xhigh, ylow, yhigh;
	switch( m_iType )
	{
	case DENSITY_NONE:
		{
			return 0.0f;
		}
		break;
	case DENSITY_GAUSSIAN:
	default:
		{
			x =  ((vPos.x - m_pOuter->GetAbsOrigin().x)/MAP_TILE_SIZE) + m_iHalfSizeX;
			y =  ((vPos.y - m_pOuter->GetAbsOrigin().y)/MAP_TILE_SIZE) + m_iHalfSizeY;
		}
		break;
	};

	xlow = floor(x);
	xhigh = ceil(x);
	ylow = floor(y);
	yhigh = ceil(y);

	if( xlow < 0 || ylow < 0 || xhigh >= m_iSizeX || yhigh >= m_iSizeY )
		return 0.0f;

	return Lerp<float>(sqrt(pow(x - xlow, 2) + pow(y - ylow, 2)), m_pWeights[xlow][ylow], m_pWeights[xhigh][yhigh]);
#else
	// Read out density for the given position
	// If outside our field, return zero (too far away).
	int x, y;
	switch( m_iType )
	{
	case DENSITY_NONE:
		{
			return 0.0f;
		}
		break;
	case DENSITY_GAUSSIANECLIPSE:
		{
			vTranslated = vPos - m_pOuter->GetAbsOrigin();
			VectorYawRotate(vTranslated, -m_pOuter->GetAbsAngles()[YAW], vTranslated);
			x =  (int)floor(((vTranslated.x + m_fXOffset)/unit_density_eclipse_tile_res.GetFloat()) + m_iHalfSizeX + 0.5f);
			y =  (int)floor(((vTranslated.y + m_fYOffset)/unit_density_eclipse_tile_res.GetFloat()) + m_iHalfSizeY + 0.5f);	
		}
		break;
	case DENSITY_GAUSSIAN:
	default:
		{
			x =  (int)floor(((vPos.x - m_pOuter->GetAbsOrigin().x)/MAP_TILE_SIZE) + m_iHalfSizeX + 0.5f);
			y =  (int)floor(((vPos.y - m_pOuter->GetAbsOrigin().y)/MAP_TILE_SIZE) + m_iHalfSizeY + 0.5f);
		}
		break;
	}

	if( x < 0 || y < 0 || x >= m_iSizeX || y >= m_iSizeY )
		return 0.0f;

	return m_pWeights[x][y];
#endif // USELERP
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DensityWeightsMap::RecalculateWeights( const Vector &vMins, const Vector &vMaxs )
{
	// Store mins/maxs
	m_vMins = vMins;
	m_vMaxs = vMaxs;

	// Clear old map
	Destroy();

	// Don't allow to generate if size is zero
	if( m_pOuter->CollisionProp()->BoundingRadius2D() == 0 )
		return;

	// Fill
	switch( m_iType )
	{
	case DENSITY_NONE:
		break;
	case DENSITY_GAUSSIAN:
		FillGaussian();
		break;
	case DENSITY_GAUSSIANECLIPSE:
		FillGaussianEclipse();
		break;
	default:
		Assert(0);
		Warning("DensityWeightsMap: Unknown density type!\n");
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DensityWeightsMap::FillGaussian()
{
	int i, j;
	float x, y;

	// Calculate sigma and grid size
	float fSigma = m_pOuter->CollisionProp()->BoundingRadius2D()*0.6f;
	m_iHalfSizeX = m_iHalfSizeY = int(fSigma*GRIDHSIZE);
	m_iSizeX = m_iSizeY = m_iHalfSizeX*2+1;

	Assert( m_iSizeX > 0 && m_iSizeY > 0 && m_iSizeX < MAX_COORD_INTEGER && m_iSizeY < MAX_COORD_INTEGER );

	//Msg("Mins: %f %f %f, Maxs: %f %f %f\n", m_vMins.x, m_vMins.y, m_vMins.z, m_vMaxs.x, m_vMaxs.y, m_vMaxs.z);
	//Msg("%s Radius: %f, Sigma: %f, halfsize: %d %d, size: %d %d\n", m_pOuter->GetClassname(), m_pOuter->CollisionProp()->BoundingRadius2D(), fSigma, m_iHalfSizeX, m_iHalfSizeY, m_iSizeX, m_iSizeY);

	// Allocate the grid
	m_pWeights = (float **)malloc(m_iSizeX*sizeof(float*));
	for(i=0; i<m_iSizeX; i++)
	{
		m_pWeights[i] = (float *)malloc(m_iSizeY*sizeof(float));
	}

	// Calculate the weights
	for(i=0; i<m_iSizeX; i++)
	{
		for(j=0; j<m_iSizeY; j++)
		{
			x = (i-m_iHalfSizeX)*MAP_TILE_SIZE;
			y = (j-m_iHalfSizeY)*MAP_TILE_SIZE;
			m_pWeights[i][j] = exp( -(((x*x) + (y*y))/(2.0f*(fSigma*fSigma))) );
		}
	}
}

float gaussian2d_ellipse(float x, float y, float theta, float sigmax, float sigmay)
{
	float xm, ym, u;
	theta = (theta/180.0f)/M_PI;
	xm = x*cos(theta) - y*sin(theta);
	ym = x*sin(theta) - y*cos(theta);
	u = pow(xm/sigmax, 2) + pow(ym/sigmay, 2);
	return exp(-u/2);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DensityWeightsMap::FillGaussianEclipse()
{
	int i, j;
	float x, y;
	float sigmax, sigmay;
	float xhsize, yhsize;
	float xoffset, yoffset;

	// Calculate parameters
	xhsize = (m_vMaxs.x - m_vMins.x)/2.0f;
	yhsize = (m_vMaxs.y - m_vMins.y)/2.0f;
	xoffset = m_vMins.x + xhsize;
	yoffset = m_vMins.y + yhsize;
	m_fXOffset = xoffset;
	m_fYOffset = yoffset;

	sigmax = xhsize*unit_density_eclipse_sigma_scale.GetFloat();//0.37f;
	sigmay = yhsize*unit_density_eclipse_sigma_scale.GetFloat();//0.37f;

	// Allocate
	float fMultiple = 32.0/unit_density_eclipse_tile_res.GetFloat();
	m_iHalfSizeX = (int)(xhsize*fMultiple);
	m_iHalfSizeY = (int)(yhsize*fMultiple);
	m_iSizeX = m_iHalfSizeX*2+1;
	m_iSizeY = m_iHalfSizeY*2+1;

	//Msg("Mins: %f %f %f, Maxs: %f %f %f\n", m_vMins.x, m_vMins.y, m_vMins.z, m_vMaxs.x, m_vMaxs.y, m_vMaxs.z);
	//Msg("halfsize: %d %d, size: %d %d. Sigmax: %f, Sigmay: %f, xoffset: %f, yoffset: %f\n", m_iHalfSizeX, m_iHalfSizeY, m_iSizeX, m_iSizeY, sigmax, sigmay, xoffset, yoffset);

	m_pWeights = (float **)malloc(m_iSizeX*sizeof(float*));
	for(i=0; i<m_iSizeX; i++)
	{
		m_pWeights[i] = (float *)malloc(m_iSizeY*sizeof(float));
	}

	// Fill
	for(i=0; i<m_iSizeX; i++)
	{
		for(j=0; j<m_iSizeY; j++)
		{
			x = (i-m_iHalfSizeX)*unit_density_eclipse_tile_res.GetFloat() - xoffset;
			y = (j-m_iHalfSizeY)*unit_density_eclipse_tile_res.GetFloat() - yoffset;

			#define BLOAT -2.0f
			if( x-BLOAT > m_vMins.x && x+BLOAT < m_vMaxs.x && y-BLOAT > m_vMins.y && y+BLOAT < m_vMaxs.y )
				m_pWeights[i][j]= 1.0f - (fabs(x)/(xhsize*unit_density_eclipse_inside_scale.GetFloat())) - 
				(fabs(y)/(yhsize*unit_density_eclipse_inside_scale.GetFloat()));
			else
				m_pWeights[i][j] = gaussian2d_ellipse(x, y, 0, sigmax, sigmay);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DensityWeightsMap::DebugPrintWeights()
{
	// Check for change.
	// Maybe do this in SetCollisionBounds?
	Vector vMins = m_pOuter->CollisionProp()->OBBMins();
	Vector vMaxs = m_pOuter->CollisionProp()->OBBMaxs();
	if( vMins != m_vMins || vMaxs != m_vMaxs )
		RecalculateWeights(vMins, vMaxs);

	int i, j;
	for(i=0; i<m_iSizeX; i++)
	{
		for(j=0; j<m_iSizeY; j++)
		{
			Msg("%f\t", m_pWeights[i][j]);
		}
		Msg("\n");
	}
}

/*
#include "unit_base_shared.h"
CON_COMMAND( density_weights_test, "" )
{
	CUnitBase *pUnit = (CUnitBase *)CreateEntityByName("unit_combine");
	pUnit->SetUnitType("unit_combine");
	DispatchSpawn(pUnit);
	DensityWeightsMap map;
	map.Init(pUnit);
	map.DebugPrintWeights();
}
*/