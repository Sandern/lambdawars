//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: 
//===========================================================================//

#include "cbase.h"
#include "src_python_te.h"
#include "src_python.h"

#ifdef CLIENT_DLL
	//#include "c_te_legacytempents.h"
	//#include "tempent.h"
#else

#endif // CLIENT_DLL


#ifdef CLIENT_DLL

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