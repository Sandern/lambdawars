from srcpy.module_generators import SemiSharedModuleGenerator
from pyplusplus.module_builder import call_policies
from pygccxml.declarations import matchers
from pyplusplus import code_creators

class EditorSystem(SemiSharedModuleGenerator):
    module_name = '_editorsystem'
    
    files = [
        'cbase.h',
        'editor/editorsystem.h',
    ]
    
    def Parse(self, mb):
        # Exclude everything by default
        mb.decls().exclude()
        
        cls = mb.class_('CEditorSystem')
        cls.include()
        cls.calldefs().virtuality = 'not virtual' 
        #cls.no_init = True
        cls.noncopyable = True
        
        mb.free_function('EditorSystem').include()
        mb.free_function('EditorSystem').call_policies = call_policies.return_value_policy(call_policies.reference_existing_object)
        
        # Finally apply common rules to all includes functions and classes, etc.
        self.ApplyCommonRules(mb)