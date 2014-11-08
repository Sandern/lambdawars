from srcpy.module_generators import SemiSharedModuleGenerator
from pyplusplus.module_builder import call_policies

class MatchMaking(SemiSharedModuleGenerator):
    module_name = 'matchmaking'
        
    files = [
        'cbase.h',
        'srcpy_matchmaking.h',
        
        'wars_matchmaking.h',
        '#wars_gameserver.h',
    ]
        
    def Parse(self, mb):
        # Exclude everything, then add what we need
        mb.decls().exclude() 
        
        mb.free_function('PyMKCreateSession').include()
        mb.free_function('PyMKCreateSession').rename('CreateSession')
        mb.free_function('PyMKMatchSession').include()
        mb.free_function('PyMKMatchSession').rename('MatchSession')
        mb.free_function('PyMKCloseSession').include()
        mb.free_function('PyMKCloseSession').rename('CloseSession')
        mb.free_function('PyMKIsSessionActive').include()
        mb.free_function('PyMKIsSessionActive').rename('IsSessionActive')

        cls = mb.class_('PyMatchSession')
        cls.include()
        cls.rename('matchsession')
        cls.mem_funs().virtuality = 'not virtual' 
        cls.mem_fun('GetSessionSystemData').call_policies = call_policies.return_value_policy( call_policies.return_by_value )  
        cls.mem_fun('GetSessionSettings').call_policies = call_policies.return_value_policy( call_policies.return_by_value )  
        
        # Wars
        mb.free_function('WarsRequestGameServer').include()
        
        if self.isserver:
            mb.enum('EGameServerState').include()
        
            mb.free_function('WarsInitGameServer').include()
            mb.free_function('WarsShutdownGameServer').include()
            mb.free_function('GetWarsGameServerState').include()
            mb.free_function('SetWarsGameServerState').include()
            mb.free_function('GetActiveGameLobbySteamID').include()
            
