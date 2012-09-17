//====== Copyright © 2007-2012 Sandern Corporation, All rights reserved. ======//
//
// Purpose: 
//===========================================================================//

#include "cbase.h"
#include "src_python_te.h"
#include "src_python.h"

#ifdef CLIENT_DLL
	#include "c_te_legacytempents.h"
	#include "tempent.h"
#else

#endif // CLIENT_DLL


#ifdef CLIENT_DLL

// Temp entity interface
static PyTempEnts g_TempEnts;
// Expose to rest of the client .dll
ITempEnts *tempents = ( ITempEnts * )&g_TempEnts;

PyTempEnts *pytempents = &g_TempEnts;

void PyTempEnts::Clear( void )
{
	CTempEnts::Clear();

	m_pyTempEnts.RemoveAll();
}

void PyTempEnts::Update( void )
{
	CTempEnts::Update();

	static int gPyTempEntFrame = 0;
	float		frametime;

	// Don't simulate while loading
	if ( ( m_pyTempEnts.Count() == 0 ) || !engine->IsInGame() )		
	{
		return;
	}

	// !!!BUGBUG	-- This needs to be time based
	gPyTempEntFrame = (gPyTempEntFrame+1) & 31;

	frametime = gpGlobals->frametime;

	int iCount = m_pyTempEnts.Count();
	C_LocalTempEntity *current;

	// in order to have tents collide with players, we have to run the player prediction code so
	// that the client has the player list. We run this code once when we detect any COLLIDEALL 
	// tent, then set this BOOL to true so the code doesn't get run again if there's more than
	// one COLLIDEALL ent for this update. (often are).

	// !!! Don't simulate while paused....  This is sort of a hack, revisit.
	if ( frametime == 0 )
	{
		for( int i=0; i < iCount; i++ )
		{
			current = boost::python::extract<C_LocalTempEntity *>(m_pyTempEnts.Element(i) );
			if( !current )
			{
				m_pyTempEnts.Remove(i);
				continue;
			}
			AddVisibleTempEntity( current );
		}
	}
	else
	{
		for( int i=iCount-1; i >= 0; i-- )
		{
			current = boost::python::extract<C_LocalTempEntity *>(m_pyTempEnts.Element(i) );

			// Kill it
			if ( !current || !current->IsActive() || !current->Frame( frametime, gPyTempEntFrame ) )
			{
				m_pyTempEnts.Remove(i);
			}
			else
			{
				// Cull to PVS (not frustum cull, just PVS)
				if ( !AddVisibleTempEntity( current ) )
				{
					if ( !( current->flags & FTENT_PERSIST ) ) 
					{
						// If we can't draw it this frame, just dump it.
						current->die = gpGlobals->curtime;
						// Don't fade out, just die
						current->flags &= ~FTENT_FADEOUT;

						m_pyTempEnts.Remove(i);
					}
				}
			}
		}
	}
}

boost::python::object PyTempEnts::PySpawnTempModel( model_t *pModel, const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecVelocity, float flLifeTime, int iFlags )
{
	Assert( pModel );
	if( !pModel )
	{
		PyErr_SetString(PyExc_Exception, "PySpawnTempModel: Invalid model reference" );
		throw boost::python::error_already_set(); 
		return boost::python::object();
	}

	// Alloc a new tempent
	C_LocalTempEntity *pTemp = PyTempEntAlloc( vecOrigin, ( model_t * ) pModel );
	if ( !pTemp )
		return boost::python::object();
	boost::python::object refTemp = m_pyTempEnts.Tail();

	pTemp->SetAbsAngles( vecAngles );
	pTemp->SetBody(0);
	pTemp->flags |= iFlags;
	pTemp->m_vecTempEntAngVelocity[0] = random->RandomFloat(-255,255);
	pTemp->m_vecTempEntAngVelocity[1] = random->RandomFloat(-255,255);
	pTemp->m_vecTempEntAngVelocity[2] = random->RandomFloat(-255,255);
	pTemp->SetRenderMode( kRenderNormal );
	pTemp->tempent_renderamt = 255;
	pTemp->SetVelocity( vecVelocity );
	pTemp->die = gpGlobals->curtime + flLifeTime;

	return refTemp;
}

boost::python::object PyTempEnts::PySpawnTempModel( const char *model_name, const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecVelocity, float flLifeTime, int iFlags )
{
	if( model_name == NULL || Q_strlen(model_name) == 0 )
	{
		PyErr_SetString(PyExc_Exception, "Name cannot be None" );
		throw boost::python::error_already_set(); 
		return boost::python::object();
	}

	model_t *pModel = (model_t *)engine->LoadModel(model_name);
	if( !pModel )
	{
		PyErr_SetString(PyExc_Exception, "PySpawnTempModel: Invalid model name" );
		throw boost::python::error_already_set(); 
		return boost::python::object();
	}
	return PySpawnTempModel(pModel, vecOrigin, vecAngles, vecVelocity, flLifeTime, iFlags);
}

C_LocalTempEntity *PyTempEnts::PyTempEntAlloc( const Vector& org, model_t *model )
{
	C_LocalTempEntity		*pTemp;

	if ( !model )
	{
		DevWarning( 1, "Can't create temporary entity with NULL model!\n" );
		return NULL;
	}

	//pTemp = TempEntAlloc();
	boost::python::object refTemp;
	try {
		refTemp = _entities.attr("C_LocalTempEntity")();
		pTemp = boost::python::extract<C_LocalTempEntity *>(refTemp);
	} catch(boost::python::error_already_set &) {
		PyErr_Print();
		PyErr_Clear();
		return NULL;
	}

	m_pyTempEnts.AddToTail( refTemp );

	pTemp->Prepare( model, gpGlobals->curtime );

	pTemp->priority = TENTPRIORITY_LOW;
	pTemp->SetAbsOrigin( org );

#ifdef HL2WARS_ASW_DLL
	pTemp->AddToLeafSystem();
#else
	pTemp->m_RenderGroup = RENDER_GROUP_OTHER;
	pTemp->AddToLeafSystem( pTemp->m_RenderGroup );
#endif // HL2WARS_ASW_DLL

	return pTemp;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
PyClientEffectRegistration *PyClientEffectRegistration::s_pHead = NULL;

PyClientEffectRegistration::PyClientEffectRegistration(const char *pEffectName, boost::python::object method) 
{
	Q_strncpy(m_EffectName, pEffectName, 128);
	m_pyMethod = method;
	m_pNext = s_pHead;
	s_pHead = this;
}

//-----------------------------------------------------------------------------
// Find our prev link. Then link prev to next.
//-----------------------------------------------------------------------------
PyClientEffectRegistration::~PyClientEffectRegistration()
{
	if( s_pHead == this )
	{
		if( m_pNext ) 
		{
			s_pHead = m_pNext;
			m_pNext = NULL;
		}
		else
		{
			s_pHead = NULL;
		}
	}
	else
	{
		PyClientEffectRegistration *pPrev = NULL;
		for ( PyClientEffectRegistration *pReg = PyClientEffectRegistration::s_pHead; pReg; pReg = pReg->m_pNext )
		{
			if( pReg == this )
			{
				if( pPrev )
					pPrev->m_pNext = m_pNext;
				break;
			}
			pPrev = pReg;
		}
	}
}

#else

#endif // CLIENT_DLL