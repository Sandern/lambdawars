from srcpy.module_generators import SemiSharedModuleGenerator
from pyplusplus.module_builder import call_policies

class Achievements(SemiSharedModuleGenerator):
    module_name = '_achievements'
    
    files = [
        'cbase.h',
        'wars_achievements.h',
        '$c_wars_steamstats.h',
    ]
    
    def Parse(self, mb):
        # Exclude everything by default
        mb.decls().exclude()
        
        # Lambda Wars achievements IDs
        mb.enum('WarsAchievements_e').include()
        
        # Lambda Wars Steam Stats manager (pretty much only used for Achievements, managed by clients)
        if self.isclient:
            cls = mb.class_('WarsUserStats_t')
            cls.include()
        
            cls = mb.class_('CWars_Steamstats')
            cls.include()
            
            cls.mem_fun('GetUserStats').call_policies = call_policies.return_internal_reference()
            
            mb.free_function('WarsSteamStats').include()
            mb.free_function('WarsSteamStats').call_policies = call_policies.return_value_policy(call_policies.reference_existing_object)