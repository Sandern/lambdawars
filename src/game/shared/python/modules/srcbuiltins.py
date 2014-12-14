from srcpy.module_generators import SharedModuleGenerator
from pyplusplus import code_creators
from pyplusplus import function_transformers as FT
from pyplusplus.module_builder import call_policies

class SrcBuiltins(SharedModuleGenerator):
    module_name = 'srcbuiltins'
    
    files = [
        'cbase.h',
        'srcpy_srcbuiltins.h',
    ]
    
    def ParseKeyValues(self, mb):
        mb.add_declaration_code( 'PyTypeObject *g_PyKeyValuesType = NULL;' )
        cls = mb.class_('KeyValues')
        self.IncludeEmptyClass(mb, 'KeyValues')
        cls.no_init = True # Destructor is private + new operator is overloaded = problems. Write a wrapper class
        cls.rename('RealKeyValues')
        cls.calldefs('KeyValues').exclude() # No constructors   
        cls.mem_opers('=').exclude() # Breaks debug mode and don't really need it
        mb.enum('types_t').include()
        
        # Wrapper class that should be used as KeyValues in python
        cls = mb.class_('PyKeyValues')
        cls.include()
        cls.rename('KeyValues')
        cls.mem_opers('=').exclude() # Breaks debug mode and don't really need it
        cls.add_registration_code('g_PyKeyValuesType = (PyTypeObject *)KeyValues_exposer.ptr();', False)

        #mb.mem_funs('GetRawKeyValues').exclude()
        mb.mem_funs('GetRawKeyValues').call_policies = call_policies.return_value_policy( call_policies.reference_existing_object )
        mb.mem_funs('GetRawKeyValues').rename('__GetRawKeyValues')
        
        # Call policies <- by value means use the converter
        mb.mem_funs('MakeCopy').call_policies = call_policies.return_value_policy(call_policies.return_by_value)  
        mb.mem_funs('CreateNewKey').call_policies = call_policies.return_value_policy(call_policies.return_by_value)  
        mb.mem_funs('FindKey').call_policies = call_policies.return_value_policy(call_policies.return_by_value)  
        mb.mem_funs('GetFirstSubKey').call_policies = call_policies.return_value_policy(call_policies.return_by_value)  
        mb.mem_funs('GetNextKey').call_policies = call_policies.return_value_policy(call_policies.return_by_value)  
        mb.mem_funs('GetFirstTrueSubKey').call_policies = call_policies.return_value_policy(call_policies.return_by_value)  
        mb.mem_funs('GetNextTrueSubKey').call_policies = call_policies.return_value_policy(call_policies.return_by_value)  
        mb.mem_funs('GetFirstValue').call_policies = call_policies.return_value_policy(call_policies.return_by_value)  
        mb.mem_funs('CreateKey').call_policies = call_policies.return_value_policy(call_policies.return_by_value)  
        mb.mem_funs('GetNextValue').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        if self.settings.branch == 'swarm':
            mb.mem_funs('FromString').call_policies = call_policies.return_value_policy(call_policies.return_by_value)  
        
        mb.free_function('KeyValuesDumpAsDevMsg').include()
        
        # Exclude vars we don't need
        mb.vars('m_sValue').exclude()
        mb.vars('m_wsValue').exclude()
        mb.vars('m_pValue').exclude()
        mb.vars('m_Color').exclude()
        
        # Add converter
        mb.add_registration_code( "ptr_keyvalues_to_py_keyvalues();" )
        mb.add_registration_code( "keyvalues_to_py_keyvalues();" )
        mb.add_registration_code( "py_keyvalues_to_keyvalues();" )
        
        mb.free_function('PyKeyValuesToDict').include()
        mb.free_function('PyKeyValuesToDict').rename('KeyValuesToDict')
        mb.free_function('PyKeyValuesToDictFromFile').include()
        mb.free_function('PyKeyValuesToDictFromFile').rename('KeyValuesToDictFromFile')
        mb.free_function('PyDictToKeyValues').include()
        mb.free_function('PyDictToKeyValues').rename('DictToKeyValues')
        mb.free_function('PyDictToKeyValues').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        
        #mb.add_registration_code( "bp::to_python_converter<\r\n\tRay_t,\r\n\tray_t_to_python_ray>();")
    
    def Parse(self, mb):
        # Exclude everything by default
        mb.decls().exclude()
        
        # Include message functions and rename them
        mb.free_function('SrcPyMsg').include()
        mb.free_function('SrcPyWarning').include()
        mb.free_function('SrcPyDevMsg').include()
        
        mb.free_function('SrcPyMsg').rename('Msg')
        mb.free_function('SrcPyWarning').rename('PrintWarning')
        mb.free_function('SrcPyDevMsg').rename('DevMsg')

        # Include classes for redirecting output (replace stdout and stderr)
        mb.class_('SrcPyStdOut').include()
        mb.class_('SrcPyStdErr').include()
        
        # Debug logging
        mb.free_function('PyCOM_TimestampedLog').include()
        mb.free_function('PyCOM_TimestampedLog').rename('COM_TimestampedLog')
        
        # Tick and per frame method register functions
        mb.free_function('RegisterTickMethod').include()
        mb.free_function('UnregisterTickMethod').include()
        mb.free_function('GetRegisteredTickMethods').include()
        mb.free_function('IsTickMethodRegistered').include()
        
        mb.free_function('RegisterPerFrameMethod').include()
        mb.free_function('UnregisterPerFrameMethod').include()
        mb.free_function('GetRegisteredPerFrameMethods').include()
        mb.free_function('IsPerFrameMethodRegistered').include()
        
        # Color classes
        cls = mb.class_('Color')
        cls.include()
        cls.mem_funs('GetColor').add_transformation( FT.output('_r'), FT.output('_g'), FT.output('_b'), FT.output('_a') )
        cls.mem_opers('=').exclude() # Breaks debug mode and don't really need it
        
        cls = mb.class_('color32_s')
        cls.include()
        cls.rename('color32')
        cls.mem_funs(allow_empty=True).exclude()
        
        if self.settings.branch == 'swarm':
            # Used by GetRenderColor in Swarm branch
            cls = mb.class_('color24')
            cls.include()
            
        self.ParseKeyValues(mb)
        
        # Global Vars Class
        mb.class_('CGlobalVarsBase').include()
        mb.class_('CGlobalVars').include()
        mb.vars('pSaveData').exclude()
        
        # Useful free functions
        mb.free_function('IsSolid').include()
        
        # Add converters
        mb.add_registration_code( "bp::to_python_converter<\n\tstring_t,\n\tstring_t_to_python_str>();")
        mb.add_registration_code( "python_str_to_string_t();" )
        mb.add_registration_code( "wchar_t_to_python_str();" )
        mb.add_registration_code( "ptr_wchar_t_to_python_str();" )
        mb.add_registration_code( "python_str_to_wchar_t();" )
        #mb.add_registration_code( "#if PY_VERSION_HEX < 0x03000000\n\tpython_unicode_to_ptr_const_wchar_t();\n\t#endif \\ PY_VERSION_HEX" )
        mb.add_registration_code( "python_unicode_to_ptr_const_wchar_t();" )
        
    def AddAdditionalCode(self, mb):
        header = code_creators.include_t( 'srcpy_srcbuiltins_converters.h' )
        mb.code_creator.adopt_include(header)
        super(SrcBuiltins, self).AddAdditionalCode(mb)
        
        
    # Adds precompiled header + other default includes
    '''def AddAdditionalCode(self, mb):
        # Add includes
        header = code_creators.include_t( 'srcpy_converters.h' )
        mb.code_creator.adopt_include(header)
        header = code_creators.include_t( 'coordsize.h' )
        mb.code_creator.adopt_include(header)
        
        super(SrcBase, self).AddAdditionalCode(mb)'''