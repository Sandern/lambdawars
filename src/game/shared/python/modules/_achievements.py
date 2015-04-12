from srcpy.module_generators import SemiSharedModuleGenerator
from pyplusplus.module_builder import call_policies

class Achievements(SemiSharedModuleGenerator):
    module_name = '_achievements'
    
    files = [
        'cbase.h',
        'wars_achievements.h',
    ]
    
    def Parse(self, mb):
        # Exclude everything by default
        mb.decls().exclude()
        
        # Lambda Wars achievements IDs
        mb.enum('WarsAchievements_e').include()