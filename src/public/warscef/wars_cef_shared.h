#ifndef WARS_CEF_SHARED_H
#define WARS_CEF_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1/strtools.h"

inline bool WarsCef_GenerateUUID( char *destuuid )
{
#ifdef WIN32
    UUID uuid;
    UuidCreate ( &uuid );

    unsigned char * str;
    if( UuidToStringA ( &uuid, &str ) != RPC_S_OK )
	{
		return false;
	}

	V_strncpy( destuuid, (char *)str, 37 );

	RpcStringFreeA ( &str );
#else
    uuid_t uuid;
    uuid_generate_random ( uuid );
    char s[37];
    uuid_unparse ( uuid, s );
#endif
	return true;
}

enum WarsCefTypes_t 
{
	WARSCEF_TYPE_JSOBJECT,
};

struct WarsCefData_t
{
	WarsCefData_t() {}
	WarsCefData_t( byte type ) : type(type) {}
	byte type;
};

struct WarsCefJSObject_t : public WarsCefData_t
{
	WarsCefJSObject_t() : WarsCefData_t( WARSCEF_TYPE_JSOBJECT ) {}
	WarsCefJSObject_t( const char *srcuuid ) : WarsCefData_t( WARSCEF_TYPE_JSOBJECT )
	{
		V_strncpy( uuid, srcuuid, sizeof( uuid ) );
	}

	char uuid[37];
};

#endif // WARS_CEF_SHARED_H