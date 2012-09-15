from generate_mods_helper import GenerateModuleSemiShared, registered_modules

from pyplusplus.module_builder import call_policies

from src_helper import *
import settings

class _SrcTests(GenerateModuleSemiShared):
    module_name = '_srctests'
    
    if settings.ASW_CODE_BASE:
        client_files = [
            'videocfg/videocfg.h',
            'cbase.h',
        ]
    else:
        client_files = [
            'wchartypes.h',
            'shake.h',
            'cbase.h',
        ]
    
    server_files = [
        'cbase.h'
    ]
    
    files = [
        'src_python_tests.h'
    ]
    
    def GetFiles(self):
        if self.isClient:
            return self.client_files + self.files 
        return self.server_files + self.files
        
    def Parse(self, mb):
        # Exclude everything by default
        mb.decls().exclude() 

        mb.free_function('SrcPyTest_EntityArg').include()
        mb.free_function('SrcPyTest_NCrossProducts').include()