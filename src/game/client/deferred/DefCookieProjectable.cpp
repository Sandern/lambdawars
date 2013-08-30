
#include "cbase.h"
#include "deferred/deferred_shared_common.h"

CDefCookieProjectable::CDefCookieProjectable( CVGUIProjectable *pProjectable )
{
	Assert( pProjectable != NULL );

	m_pProjectable = pProjectable;
}

CDefCookieProjectable::~CDefCookieProjectable()
{
	delete m_pProjectable;
}

ITexture *CDefCookieProjectable::GetCookieTarget( const int iTargetIndex )
{
	return GetProjectableVguiRT( iTargetIndex );
}

void CDefCookieProjectable::PreRender( const int iTargetIndex )
{
	m_pProjectable->DrawSelfToRT( GetProjectableVguiRT( iTargetIndex ) );
}