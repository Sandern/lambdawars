#! /usr/bin/python
''' src_module_builder provides a builder set up for building modules for source sdk code.
'''

from pyplusplus import module_builder
from pygccxml import parser

import platform
import os

# Include directories
incl_paths = [
    # GCCXML Support files
    '../../srcpypp/Support/GCC/4.6',
    
    # System Headers/Default include paths by GCC (basically a copy of the usr folder of Ubuntu 10.04 32 bit)
    '../../srcpypp/Support/GCC/4.6/usr/lib/gcc/i686-linux-gnu/4.6/include',
    '../../srcpypp/Support/GCC/4.6/usr/lib/gcc/i686-linux-gnu/4.6/include-fixed',
    '../../srcpypp/Support/GCC/4.6/usr/include/c++/4.6',
    '../../srcpypp/Support/GCC/4.6/usr/include/c++/4.6/i686-linux-gnu',
    '../../srcpypp/Support/GCC/4.6/usr/include/i386-linux-gnu',
    '../../srcpypp/Support/GCC/4.6/usr/include',
]

incl_paths_client = [
    '../../game/client', 
    '../../game/client/python',
    '../../game/client/hl2wars', 
    '../../game/client/cef',
    '../../cef',
]
incl_paths_server = [
    '../../game/server', 
    '../../game/server/python',
    '../../game/server/hl2wars', 
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
    'ENABLE_CEF',
    'ENABLE_PYTHON',

    # Just pretend SSE and MMX are enabled
    '__SSE__',
    '__MMX__',
    
    # Generation
    'PYPP_GENERATION',
]

# Defined symbols
basesymbols = [
    '__SSE__',
    '__MMX__',
    
    # Generation
    'PYPP_GENERATION',
]

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
ARCH = 'x86_64'
ARCH_CFLAGS = '-mtune=i686 -march=pentium3 -mmmx -m32 '
BASE_CFLAGS = '-Wall -Wextra -Wshadow -Wno-invalid-offsetof -fno-strict-aliasing -Wno-unknown-pragmas -Wno-unused-parameter -Wno-unused-value -Wno-missing-field-initializers -Wno-sign-compare -Wno-reorder -Wno-invalid-offsetof -Wno-float-equal -Werror=return-type -fdiagnostics-show-option -Wformat -Wformat-security -fpermissive '
SHLIBEXT = 'so'
SHLIBCFLAGS = '-fPIC '
SHLIBLDFLAGS = '-shared -Wl,-Map,$@_map.txt -Wl '
default_cflags = ARCH_CFLAGS+BASE_CFLAGS+SHLIBCFLAGS#+'-fpermissive '

if platform.system() == 'Windows':
    gccbinpath = '../../srcpypp/gccxml_bin/win32'
elif platform.system() == 'Linux':
    gccbinpath = '../../srcpypp/gccxml_bin/linux'
else:
    gccbinpath = '../../srcpypp/gccxml_bin/mac'

# NOTE: module_builder_t == builder_t
class src_module_builder_t(module_builder.module_builder_t):
    """
    This is basically a wrapper module with the arguments already setted up for source.
    """

    def __init__(   self
                    , files
                    , includes
                    , symbols
                    , is_client = False
                ): 
        symbols = list(basesymbols + symbols)
        
        includepaths = incl_paths + includes

        includepaths = list(map(lambda p: p.replace('\\', '/'), includepaths))
        includepaths = list(filter(os.path.normpath, includepaths))
            
        self.definedsymbols = symbols
        self.includes = includepaths
        
        module_builder.module_builder_t.__init__(
                    self
                    , files
                    , gccxml_path=gccbinpath
                    , working_directory='.'
                    , include_paths=includepaths
                    , define_symbols=symbols
                    , undefine_symbols=usymbols
                    , start_with_declarations=None
                    , compilation_mode=parser.COMPILATION_MODE.ALL_AT_ONCE
                    , cache=None
                    , optimize_queries=True
                    , ignore_gccxml_output=False
                    , indexing_suite_version=1
                    , cflags=default_cflags+' --gccxml-config "../../srcpypp/gccxml_config" --gccxml-gcc-options "../../srcpypp/gccxml_gcc_options"'
                    , encoding='ascii'
                    )
