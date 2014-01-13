from srcpy.module_generators import ClientModuleGenerator

from pyplusplus.module_builder import call_policies
from pyplusplus import function_transformers as FT
from pyplusplus import code_creators
from pygccxml.declarations import matchers

class Input(ClientModuleGenerator):
    module_name = '_input'

    files = [
        'vgui/Cursor.h',
        'inputsystem/ButtonCode.h',
        'kbutton.h',
        'cbase.h',
        'input.h',
    ]

    def Parse(self, mb):
        # Exclude everything by default
        mb.decls().exclude()
        
        mb.class_('kbutton_t').include()
        if self.settings.branch == 'swarm':
            mb.class_('kbutton_t').mem_funs('GetPerUser').exclude()
        
        # Include input class
        cls = mb.class_('CInput')
        cls.include()
        cls.mem_funs().virtuality = 'not virtual'
        if self.settings.branch == 'swarm':
            #mb.mem_funs('FindKey').exclude() # FIXME
            mb.mem_funs('FindKey').call_policies = call_policies.return_value_policy( call_policies.reference_existing_object ) 
        else:
            mb.mem_funs('FindKey').call_policies = call_policies.return_value_policy( call_policies.manage_new_object )  
        mb.mem_funs('GetUserCmd').call_policies = call_policies.return_value_policy( call_policies.reference_existing_object ) # Can't exclude due being abstract
        
        mb.mem_funs( 'CAM_OrthographicSize' ).add_transformation( FT.output('w'), FT.output('h') )
        
        mb.add_declaration_code( "CInput *wrap_Input()\r\n{\r\n\treturn (CInput *)::input;\r\n}\r\n" )
        mb.add_registration_code( 'bp::def( "input", wrap_Input, bp::return_value_policy<bp::reference_existing_object>() );' )
        
        # ButtonCode.  
        mb.enums('ButtonCode_t').include()
        
        # Remove any protected function 
        mb.calldefs( matchers.access_type_matcher_t( 'protected' ) ).exclude()
