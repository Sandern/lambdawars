#ifndef DEF_COOKIE_TEXTURE_H
#define DEF_COOKIE_TEXTURE_H

#include "deferred/deferred_shared_common.h"

class CDefCookieTexture : public IDefCookie
{
public:

	CDefCookieTexture( ITexture *pTex );

	virtual ITexture *GetCookieTarget( const int iTargetIndex );

private:

	ITexture *m_pTexture;
};


#endif