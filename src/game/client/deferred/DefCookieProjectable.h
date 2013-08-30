#ifndef DEF_COOKIE_PROJECTABLE_H
#define DEF_COOKIE_PROJECTABLE_H

#include "deferred/deferred_shared_common.h"

class CVGUIProjectable;

class CDefCookieProjectable : public IDefCookie
{
public:

	CDefCookieProjectable( CVGUIProjectable *pProjectable );
	~CDefCookieProjectable();

	virtual ITexture *GetCookieTarget( const int iTargetIndex );
	virtual void PreRender( const int iTargetIndex );

private:

	CVGUIProjectable *m_pProjectable;
};


#endif