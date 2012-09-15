from generate_mods_helper import GenerateModuleSemiShared
from src_helper import DisableKnownWarnings
import settings

from pyplusplus.module_builder import call_policies
from pygccxml.declarations import matchers
from pyplusplus import messages

class NDebugOverlay(GenerateModuleSemiShared):
    module_name = '_ndebugoverlay'
    
    if settings.ASW_CODE_BASE:
        client_files = [
            'videocfg/videocfg.h',
        ]
    else:
        client_files = [
            'mathlib/vector.h',
            'wchartypes.h',
            'shake.h',
        ]
    
    files = [
        'cbase.h',
        'debugoverlay_shared.h'
    ]
    def GetFiles(self):
        if self.isClient:
            return self.client_files + self.files 
        return self.files 

    def Parse(self, mb):
        # Exclude everything, then add what we need
        # Otherwise we get very big source code and dll's
        mb.decls().exclude()  
        
        mb.add_declaration_code( 'class CBaseEntity;\r\n')
        
        mb.namespace('NDebugOverlay').include()
        mb.free_functions('DrawOverlayLines').exclude()
        
        # Remove any protected function 
        #mb.calldefs( matchers.access_type_matcher_t( 'protected' ) ).exclude()
        
        # Shut up about warnings of generating class wrappers
        mb.classes().disable_warnings( messages.W1027 )
        
        # Disable shared warnings
        DisableKnownWarnings(mb)
    