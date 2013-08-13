from srcpy.module_generators import ClientModuleGenerator
from src_helper import *
import settings

from pyplusplus.module_builder import call_policies
from pyplusplus import function_transformers as FT
from pyplusplus import code_creators
from pygccxml.declarations import matchers

class Input(ClientModuleGenerator):
    module_name = '_input'

    if settings.ASW_CODE_BASE:
        files = [
            'videocfg/videocfg.h',

            'vgui/Cursor.h',
            'inputsystem/ButtonCode.h',
            'kbutton.h',
            'cbase.h',
            'input.h',
        ]
    else:
        files = [
            'wchartypes.h',
            'shake.h',
        
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
        if settings.ASW_CODE_BASE:
            mb.class_('kbutton_t').mem_funs('GetPerUser').exclude()
        
        # //--------------------------------------------------------------------------------------------------------------------------------
        # Include iinput class
        cls = mb.class_('CInput')
        cls.include()
        cls.mem_funs().virtuality = 'not virtual'
        if settings.ASW_CODE_BASE:
            #mb.mem_funs('FindKey').exclude() # FIXME
            mb.mem_funs('FindKey').call_policies = call_policies.return_value_policy( call_policies.reference_existing_object ) 
        else:
            mb.mem_funs('FindKey').call_policies = call_policies.return_value_policy( call_policies.manage_new_object )  
        mb.mem_funs('GetUserCmd').call_policies = call_policies.return_value_policy( call_policies.reference_existing_object )  # Can't exclude, since abstract
        
        mb.mem_funs( 'CAM_OrthographicSize' ).add_transformation( FT.output('w'), FT.output('h') )
        
        mb.add_declaration_code( "CInput *wrap_Input()\r\n{\r\n\treturn (CInput *)::input;\r\n}\r\n" )
        mb.add_registration_code( 'bp::def( "input", wrap_Input, bp::return_value_policy<bp::reference_existing_object>() );' )
        
        # //--------------------------------------------------------------------------------------------------------------------------------
        # ButtonCode.
        mb.enums('ButtonCode_t').disable_warnings( messages.W1032 )    
        mb.enums('ButtonCode_t').include()
        
        # Remove any protected function 
        mb.calldefs( matchers.access_type_matcher_t( 'protected' ) ).exclude()
            
        # Disable shared warnings
        DisableKnownWarnings(mb)
