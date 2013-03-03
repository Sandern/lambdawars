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
        #'matchmaking/imatchframework.h',
    ]
    
    def GetFiles(self):
        if self.isClient:
            return self.client_files + self.files 
        return self.files
        
    def Parse(self, mb):
        # Exclude everything, then add what we need
        mb.decls().exclude() 
        
        
        mb.free_function('PyMKCreateSession').include()
        mb.free_function('PyMKCreateSession').rename('CreateSession')
        mb.free_function('PyMKMatchSession').include()
        mb.free_function('PyMKMatchSession').rename('MatchSession')
        mb.free_function('PyMKCloseSession').include()
        mb.free_function('PyMKCloseSession').rename('CloseSession')

        cls = mb.class_('PyMatchSession')
        cls.include()
        cls.rename('matchsession')
        cls.mem_fun('GetSessionSystemData').call_policies = call_policies.return_value_policy( call_policies.return_by_value )  
        cls.mem_fun('GetSessionSettings').call_policies = call_policies.return_value_policy( call_policies.return_by_value )  
        
        cls = mb.class_('PyMatchSystem')
        cls.include()
        cls.rename('matchsystem')
        
        cls = mb.class_('PySearchManager')
        cls.include()
        cls.rename('SearchManager')
        cls.mem_fun('SetSearchManagerInternal').exclude()
