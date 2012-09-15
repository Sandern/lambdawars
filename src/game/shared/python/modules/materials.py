from generate_mods_helper import GenerateModuleSemiShared
import settings

from pyplusplus.module_builder import call_policies
from pygccxml.declarations import matchers

class Materials(GenerateModuleSemiShared):
    module_name = 'materials'
    
    if settings.ASW_CODE_BASE:
        client_files = ['videocfg/videocfg.h']
    else:
        client_files = ['wchartypes.h','shake.h']
        
    client_files.extend( [
        'cdll_client_int.h',
        'viewpostprocess.h',
    ] )
        
    server_files = []
    
    files = [
        'cbase.h',
        'materialsystem\imaterial.h',
        'materialsystem\MaterialSystemUtil.h',
        'python\src_python_materials.h',
        #'avi/ibik.h',
    ]
    def GetFiles(self):
        if self.isClient:
            return self.client_files + self.files  + ['glow_outline_effect.h']
        return self.server_files + self.files
        
    def Parse(self, mb):
        # Exclude everything, then add what we need
        # Otherwise we get very big source code and dll's
        mb.decls().exclude()  

        # Material reference
        cls = mb.class_('CMaterialReference')
        cls.include()
        mb.global_ns.mem_opers('*').call_policies = call_policies.return_value_policy( call_policies.reference_existing_object )
        #mb.global_ns.mem_opers('::IMaterial const *').call_policies = call_policies.return_value_policy( call_policies.reference_existing_object )
        #mb.global_ns.casting_operators( '*' ).call_policies = call_policies.return_value_policy( call_policies.reference_existing_object )
        mb.global_ns.mem_opers().exclude() # Fuck them for now
        mb.global_ns.casting_operators().exclude()

        # if self.isClient:
            # cls = mb.class_('IAppSystem')
            # cls.mem_funs().virtuality = 'not virtual'
            
            # # bik interface        
            # cls = mb.class_('IBik')
            # cls.include()
            # cls.mem_funs().virtuality = 'not virtual'
            
            # cls.mem_fun('GetMaterial').call_policies = call_policies.return_value_policy( call_policies.return_by_value )
            
            # # Lame fix, don't know how to change the order of the registrations...
            # cls.add_registration_code( ";}\r\nbp::scope().attr( \"bik\" ) = boost::ref(bik);{", False )
            # #mb.add_registration_code( "bp::scope().attr( \"bik\" ) = boost::ref(bik);" )
            
        if self.isClient:
            if settings.ASW_CODE_BASE:
                mb.free_function('PyIsDeferredRenderingEnabled').include()
                mb.free_function('PyIsDeferredRenderingEnabled').rename('IsDeferredRenderingEnabled')
            
            cls = mb.class_('CGlowObjectManager')
            cls.include()
            cls.mem_funs().virtuality = 'not virtual'
            cls.no_init = True
            
            mb.add_registration_code( "bp::scope().attr( \"glowobjectmanager\" ) = boost::ref(g_GlowObjectManager);" )
            
            cls = mb.class_('PyProceduralTexture')
            cls.rename('ProceduralTexture')
            cls.include()
            
            mb.enum('ImageFormat').include()

        
        # Remove any protected function 
        mb.calldefs( matchers.access_type_matcher_t( 'protected' ) ).exclude()
    