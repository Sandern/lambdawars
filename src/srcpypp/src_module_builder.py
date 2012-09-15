#! /usr/bin/python
# src_module_builder provides a builder setted up for building modules for source sdk code

from pyplusplus import module_builder
from pygccxml import parser
import settings

# Include directories
incl_paths = [
    'gccxml_bin/v09/linux2/share/gccxml-0.9/GCC/4.2',
    
    # System headers
    'usr/include/c++/4.3',
    'usr/include/c++/4.3/i486-linux-gnu',
    'usr/include/c++/4.3/backward',
    'usr/local/include',
    'usr/lib/gcc/i486-linux-gnu/4.3/include',
    'usr/lib/gcc/i486-linux-gnu/4.3/include-fixed',
    './usr/include',
    
    # Game
    '../common',
    '../public',
    '../public/tier0',
    '../public/tier1',
    '../game/shared',
    #'../utils/common',
    '../game/shared/hl2wars',
    #'../game/shared/multiplayer',
    '../game/shared/python',
    
    # Python/Boost folders
    '../python/Include',
    '../python',
    
    # Too much work to fix things to let boost python be parsed correctly
    # We only need to know the object class one anyway
    #'../boost',
    '../srcpypp/boost_stubs',
]

incl_paths_client = [
    '../game/client/hl2wars', 
    '../game/client', 
    '../game/client/python',
    '../Awesomium',
]
incl_paths_server = [
    '../game/server/hl2wars', 
    '../game/server', 
    '../game/server/python',
]

# Defined symbols
dsymbols = [
    'NDEBUG',
    '_USRDLL',
    '_CRT_SECURE_NO_DEPRECATE',
    '_CRT_NONSTDC_NO_DEPRECATE',
    'VECTOR',
    'PROTECTED_THINGS_ENABLE',
    'sprintf=use_Q_snprintf_instead_of_sprintf',
    'strncpy=use_Q_strncpy_instead'
    '_snprintf=use_Q_snprintf_instead',
    'USES_SAVERESTORE',
    'VERSION_SAFE_STEAM_API_INTERFACES',
    'ARCH=i486',
    '_LINUX',
    'LINUX',
    'COMPILER_GCC',
    '_POSIX',
    'POSIX',
    'VPROF_LEVEL=1',
    'SWDS',
    '_finite=finite',
    'stricmp=strcasecmp',
    '_stricmp=strcasecmp',
    '_strnicmp=strncasecmp',
    'strnicmp=strncasecmp',
    '_vsnprintf=vsnprintf',
    '_alloca=alloca',
    'strcmpi=strcasecmp',
    
    '_strtoi64=strtoll',
    'VPROF_ENABLED',
    
    'BOOST_AUTO_LINK_NOMANGLE',
    
    'HL2WARS_DLL',

    # Just pretend SSE and MMX are enabled
    '__SSE__',
    '__MMX__',
    
    # Generation
    'PYPP_GENERATION',
]

if settings.ASW_CODE_BASE:
    dsymbols.append( 'HL2WARS_ASW_DLL' )

dsymbols_client = [ 
    'CLIENT_DLL' 
]
dsymbols_server = [
    'GAME_DLL' 
]

# Undefined symbols
usymbols = [
    'sprintf=use_Q_snprintf_instead_of_sprintf',
    'strncpy=use_Q_strncpy_instead',
    'fopen=dont_use_fopen',
    'PROTECTED_THINGS_ENABLE',
]

# cflags
ARCH = 'i486'
ARCH_CFLAGS = '-mtune=i686 -march=pentium3 -mmmx -m32 '
BASE_CFLAGS = '-fno-strict-aliasing -Wall -Wconversion -Wno-non-virtual-dtor -Wno-invalid-offsetof '
SHLIBEXT = 'so'
SHLIBCFLAGS = '-fPIC '
SHLIBLDFLAGS = '-shared -Wl,-Map,$@_map.txt -Wl '
default_cflags = ARCH_CFLAGS+BASE_CFLAGS+SHLIBCFLAGS+'-fpermissive '

# NOTE: module_builder_t == builder_t
class src_module_builder_t(module_builder.module_builder_t):
    """
    This is basically a wrapper module with the arguments already setted up for source.
    """

    def __init__(   self
                    , files
                    , is_client = False
                ):
        if is_client:
            ds = dsymbols+dsymbols_client
            incl = incl_paths_client+incl_paths
        else:
            ds = dsymbols+dsymbols_server
            incl = incl_paths_server+incl_paths
        #files.insert( 0,'gccxml_bin/v09/linux2/share/gccxml-0.9/GCC/4.2/gccxml_builtins.h')      
        
        module_builder.module_builder_t.__init__(
                    self
                    , files
                    , gccxml_path='gccxml_bin/v09/win32/bin'
                    , working_directory='.'
                    , include_paths=incl
                    , define_symbols=ds
                    , undefine_symbols=usymbols
                    , start_with_declarations=None
                    , compilation_mode=parser.COMPILATION_MODE.ALL_AT_ONCE
                    , cache=None
                    , optimize_queries=True
                    , ignore_gccxml_output=False
                    , indexing_suite_version=1
                    , cflags=default_cflags+' --gccxml-config gccxml_config'
                    , encoding='ascii'
                    , compiler=None
                    , gccxml_config=None
                    )
