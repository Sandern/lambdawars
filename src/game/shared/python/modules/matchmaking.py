from srcpy.module_generators import SemiSharedModuleGenerator
from pyplusplus.module_builder import call_policies

class MatchMaking(SemiSharedModuleGenerator):
    module_name = 'matchmaking'
        
    files = [
        'cbase.h',
        'srcpy_matchmaking.h',
        #'steam/isteammatchmaking.h',
        #'matchmaking/imatchframework.h',
        
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

        '''cls = mb.class_('PyMatchEventsSubscription')
        cls.include()
        cls.rename('matcheventssubscription')
        cls.mem_funs().virtuality = 'not virtual'
        cls.mem_fun('GetEventData').call_policies = call_policies.return_value_policy( call_policies.return_by_value )  '''
        
        cls = mb.class_('PyMatchSession')
        cls.include()
        cls.rename('matchsession')
        cls.mem_funs().virtuality = 'not virtual' 
        cls.mem_fun('GetSessionSystemData').call_policies = call_policies.return_value_policy( call_policies.return_by_value )  
        cls.mem_fun('GetSessionSettings').call_policies = call_policies.return_value_policy( call_policies.return_by_value )  
        
        '''cls = mb.class_('PyMatchSystem')
        cls.include()
        cls.rename('matchsystem')
        cls.mem_funs().virtuality = 'not virtual' 
        
        cls = mb.class_('PySearchManager')
        cls.include()
        cls.rename('SearchManager')
        #cls.calldefs('PySearchManager').exclude()
        cls.mem_funs().virtuality = 'not virtual' 
        cls.mem_fun('SetSearchManagerInternal').exclude()
        
        cls = mb.class_('PyMatchSearchResult')
        cls.include()
        cls.rename('MatchSearchResult')
        cls.mem_funs().virtuality = 'not virtual'
        #cls.calldefs('PyMatchSearchResult').exclude()
        cls.mem_fun('GetGameDetails').call_policies = call_policies.return_value_policy( call_policies.return_by_value )  
        
        cls = mb.class_('PyMatchEventsSink')
        cls.include()
        cls.rename('MatchEventsSink')

        cls = mb.class_('PySteamMatchmaking')
        cls.include()
        cls.rename('steammatchmaking')'''
        
        # Wars
        mb.free_function('WarsRequestGameServer').include()
        
        if self.isserver:
            mb.enum('EGameServerState').include()
        
            mb.free_function('WarsInitGameServer').include()
            mb.free_function('WarsShutdownGameServer').include()
            mb.free_function('GetWarsGameServerState').include()
