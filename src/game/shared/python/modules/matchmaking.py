from generate_mods_helper import GenerateModuleSemiShared
from pyplusplus.module_builder import call_policies
import settings
#from src_helper import *

class MatchMaking(GenerateModuleSemiShared):
    module_name = 'matchmaking'
    
    if settings.ASW_CODE_BASE:
        client_files = [
            'videocfg/videocfg.h',
        ]
    else:
        client_files = [
            'wchartypes.h',
            'shake.h',
        ]
        
    files = [
        'cbase.h',
        'src_python_matchmaking.h',
    ]
    
    def GetFiles(self):
        if self.isClient:
            return self.client_files + self.files 
        return self.files
        
    def Parse(self, mb):
        # Exclude everything, then add what we need
        # Otherwise we get very big source code and dll's
        mb.decls().exclude() 
        
        mb.free_function('PyMKCreateSession').include()
        mb.free_function('PyMKCreateSession').rename('CreateSession')
        mb.free_function('PyMKMatchSession').include()
        mb.free_function('PyMKMatchSession').rename('MatchSession')

        cls = mb.class_('PyMatchSession')
        cls.include()
        cls.rename('matchsession')
        cls.mem_fun('GetSessionSystemData').call_policies = call_policies.return_value_policy( call_policies.return_by_value )  
        cls.mem_fun('GetSessionSettings').call_policies = call_policies.return_value_policy( call_policies.return_by_value )  
        