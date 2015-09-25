#!/usr/bin/env python3

tables = [
    ('DT_BaseEntity', 32),
    ('DT_BaseAnimating', 32),
    ('DT_BaseAnimatingOverlay', 2),
    ('DT_BaseFlex', 8),
    ('DT_BaseCombatCharacter', 20),
    ('DT_BasePlayer', 2),
    ('DT_BaseProjectile', 16),
    ('DT_BaseGrenade', 16),
    ('DT_BaseCombatWeapon', 2),
    ('DT_PlayerResource', 1),
    ('DT_BreakableProp', 2),
    ('DT_BaseToggle', 2),
    ('DT_BaseTrigger', 16),
    
    # Seldom creating new effect classes. Should use the particle system anyway.
    ('DT_Beam', 2),
    ('DT_Sprite', 2),
    ('DT_SmokeTrail', 2),
    
    ('DT_HL2WarsPlayer', 4),
    ('DT_UnitBase', 128),
    ('DT_WarsWeapon', 20),
    ('DT_FuncUnit', 12),
    ('DT_BaseFuncMapBoundary', 2),
]

path = '../game/shared/python/srcpy_scclasses.cpp'

template = '''// Autogenerated file
#include "cbase.h"
#include "srcpy.h"
//#include "srcpy_boostpython.h"
#ifdef CLIENT_DLL
#include "srcpy_client_class.h"
#else
#include "srcpy_server_class.h"
#endif
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
%(client_entries)s
#else
%(server_entries)s
#endif // CLIENT_DLL
'''

client_entries = ''
server_entries = ''

for e in tables:
    client_entries += '\tEXTERN_RECV_TABLE( %s );\n' % e[0]
    server_entries += '\tEXTERN_SEND_TABLE( %s );\n' % e[0]
    for i in range(0, e[1]):
        args = {'table': e[0], 'num': i}
        client_entries += '\tIMPLEMENT_PYCLIENTCLASS_SYSTEM( CPY%(table)s%(num)d, PY%(table)s%(num)d, &(%(table)s::g_RecvTable) );\n' % args
        server_entries += '\tIMPLEMENT_PYSERVERCLASS_SYSTEM( SPY%(table)s%(num)d, PY%(table)s%(num)d, &(%(table)s::g_SendTable) );\n' % args

with open(path, 'w') as fp:
    fp.write(template % {'client_entries': client_entries, 'server_entries': server_entries})
