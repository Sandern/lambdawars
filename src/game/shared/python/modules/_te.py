from generate_mods_helper import GenerateModuleSemiShared, registered_modules
from src_helper import *
import settings

from pyplusplus.module_builder import call_policies
from pyplusplus import function_transformers as FT
from pygccxml.declarations import matchers

class TE(GenerateModuleSemiShared):
    module_name = '_te'
    
    client_files = [
        'videocfg/videocfg.h',

        'cbase.h',
        'tempent.h',
        'c_te_legacytempents.h',
        'c_te_effect_dispatch.h',
        'fx.h',
        'fx_quad.h',
        'fx_line.h',
        'clientsideeffects.h',
        'wars_mesh_builder.h',
        'fx_envelope.h',
        'c_strider_fx.h',
    ]
    
    server_files = [
        'cbase.h',
        'te_effect_dispatch.h',
    ]
    
    files = [
        'effect_dispatch_data.h',
        'src_python_te.h',
    ]
    
    def GetFiles(self):
        if self.isClient:
            return self.client_files + self.files 
        return self.server_files + self.files 

    def ParseServer(self, mb):
        mb.class_('CEffectData').vars('m_nEntIndex').exclude()
        
        mb.free_function('CreateConcussiveBlast').include()
        
    def ParseClient(self, mb):
        # Don't care
        mb.class_('CEffectData').mem_funs().exclude()
        mb.class_('CEffectData').vars('m_hEntity').exclude()
        
        # Registering new effects
        cls = mb.class_('PyClientEffectRegistration')
        cls.include()
        cls.rename('ClientEffectRegistration')
        cls.vars().exclude()
        
        # Functions to do some effects
        mb.free_functions('FX_AddQuad').include()
        
        # fx.h
        mb.free_functions('FX_RicochetSound').include()
        mb.free_functions('FX_AntlionImpact').include()
        mb.free_functions('FX_DebrisFlecks').include()
        mb.free_functions('FX_Tracer').include()
        mb.free_functions('FX_GunshipTracer').include()
        mb.free_functions('FX_StriderTracer').include()
        mb.free_functions('FX_HunterTracer').include()
        mb.free_functions('FX_PlayerTracer').include()
        #mb.free_functions('FX_BulletPass').include()
        mb.free_functions('FX_MetalSpark').include()
        mb.free_functions('FX_MetalScrape').include()
        mb.free_functions('FX_Sparks').include()
        mb.free_functions('FX_ElectricSpark').include()
        mb.free_functions('FX_BugBlood').include()
        mb.free_functions('FX_Blood').include()
        #mb.free_functions('FX_CreateImpactDust').include()
        mb.free_functions('FX_EnergySplash').include()
        mb.free_functions('FX_MicroExplosion').include()
        mb.free_functions('FX_Explosion').include()
        mb.free_functions('FX_ConcussiveExplosion').include()
        mb.free_functions('FX_DustImpact').include()
        mb.free_functions('FX_MuzzleEffect').include()
        mb.free_functions('FX_MuzzleEffectAttached').include()
        mb.free_functions('FX_StriderMuzzleEffect').include()
        mb.free_functions('FX_GunshipMuzzleEffect').include()
        mb.free_functions('FX_Smoke').include()
        mb.free_functions('FX_Dust').include()
        #mb.free_functions('FX_CreateGaussExplosion').include()
        mb.free_functions('FX_GaussTracer').include()
        mb.free_functions('FX_TracerSound').include()

        # Temp Ents
        cls = mb.class_('CTempEnts')
        cls.include()
        cls.mem_funs().virtuality = 'not virtual' 
        cls.calldefs( matchers.access_type_matcher_t( 'protected' ), allow_empty=True ).exclude()
        
        cls = mb.class_('PyTempEnts')
        cls.include()
        cls.calldefs().virtuality = 'not virtual' 
        cls.calldefs( matchers.access_type_matcher_t( 'protected' ), allow_empty=True ).exclude()
        
        mb.add_registration_code( 'bp::scope().attr( "tempents" ) = boost::ref(pytempents);' )
        
        mb.mem_funs('PySpawnTempModel').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        mb.mem_funs('DefaultSprite').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        mb.mem_funs('TempSprite').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        mb.mem_funs('ClientProjectile').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        
        mb.mem_funs('RicochetSprite').exclude()
        mb.mem_funs('SpawnTempModel').exclude()
        #mb.mem_funs('PySpawnTempModel').exclude()
        mb.mem_funs('PySpawnTempModel').rename('SpawnTempModel')
        mb.mem_funs('ClientProjectile').exclude() # Debug mode problem
        mb.mem_funs('DefaultSprite').exclude() # Debug mode problem
        mb.mem_funs('TempSprite').exclude() # Debug mode problem
        
        # Mesh builder
        cls = mb.class_('PyMeshVertex')
        cls.include()
        cls.rename('MeshVertex')
        
        cls.var('m_hEnt').exclude()
        cls.mem_funs('GetEnt').call_policies = call_policies.return_value_policy( call_policies.return_by_value ) 
        cls.add_property( 'ent'
                         , cls.member_function( 'GetEnt' )
                         , cls.member_function( 'SetEnt' ) )
                         
        cls = mb.class_('PyMeshBuilder')
        cls.include()
        cls.rename('MeshBuilder')
        cls.mem_funs().virtuality = 'not virtual' 
        
        mb.enum('MaterialPrimitiveType_t').include()
        
        # Add client effects class (you can only add mesh builders to it)
        cls = mb.class_('PyClientSideEffect')
        cls.include()
        cls.rename('ClientSideEffect')
        cls.mem_funs().virtuality = 'not virtual' 
        cls.mem_funs('AddToEffectList').exclude()
        cls.mem_funs('Draw').virtuality = 'virtual'
        
        cls.add_registration_code( 
        ('{ //::PyClientSideEffect::IsActive\r\n'
        '\r\n'
        '    typedef bool ( ::PyClientSideEffect::*IsActive_function_type )(  ) ;\r\n'
        '    \r\n'
        '    ClientSideEffect_exposer.def( \r\n'
        '        "IsActive"\r\n'
        '        , IsActive_function_type( &::PyClientSideEffect::IsActive ) );\r\n'
        '\r\n'
        '}\r\n'), False)
        
        cls.add_registration_code( 
        ('{ //::PyClientSideEffect::Destroy\r\n'
        '\r\n'
        '    typedef void ( ::PyClientSideEffect::*Destroy_function_type )(  ) ;\r\n'
        '    \r\n'
        '    ClientSideEffect_exposer.def( \r\n'
        '        "Destroy"\r\n'
        '        , Destroy_function_type( &::PyClientSideEffect::Destroy ) );\r\n'
        '\r\n'
        '}\r\n'), False)
        
        mb.free_function('AddToClientEffectList').include()
        
        # FX Envelope + strider fx
        cls = mb.class_('C_EnvelopeFX')
        cls.include()
        cls.mem_funs().virtuality = 'not virtual' 
        
        cls = mb.class_('C_StriderFX')
        cls.include()
        cls.mem_funs().virtuality = 'not virtual' 
        
        # //--------------------------------------------------------------------------------------------------------------------------------
        mb.add_registration_code( "bp::scope().attr( \"FTENT_NONE\" ) = (int)FTENT_NONE;" )
        mb.add_registration_code( "bp::scope().attr( \"FTENT_SINEWAVE\" ) = (int)FTENT_SINEWAVE;" )
        mb.add_registration_code( "bp::scope().attr( \"FTENT_GRAVITY\" ) = (int)FTENT_GRAVITY;" )
        mb.add_registration_code( "bp::scope().attr( \"FTENT_ROTATE\" ) = (int)FTENT_ROTATE;" )
        mb.add_registration_code( "bp::scope().attr( \"FTENT_SLOWGRAVITY\" ) = (int)FTENT_SLOWGRAVITY;" )
        mb.add_registration_code( "bp::scope().attr( \"FTENT_SMOKETRAIL\" ) = (int)FTENT_SMOKETRAIL;" )
        mb.add_registration_code( "bp::scope().attr( \"FTENT_COLLIDEWORLD\" ) = (int)FTENT_COLLIDEWORLD;" )
        mb.add_registration_code( "bp::scope().attr( \"FTENT_FLICKER\" ) = (int)FTENT_FLICKER;" )
        mb.add_registration_code( "bp::scope().attr( \"FTENT_FADEOUT\" ) = (int)FTENT_FADEOUT;" )
        mb.add_registration_code( "bp::scope().attr( \"FTENT_SPRANIMATE\" ) = (int)FTENT_SPRANIMATE;" )
        mb.add_registration_code( "bp::scope().attr( \"FTENT_HITSOUND\" ) = (int)FTENT_HITSOUND;" )
        mb.add_registration_code( "bp::scope().attr( \"FTENT_SPIRAL\" ) = (int)FTENT_SPIRAL;" )
        mb.add_registration_code( "bp::scope().attr( \"FTENT_SPRCYCLE\" ) = (int)FTENT_SPRCYCLE;" )
        mb.add_registration_code( "bp::scope().attr( \"FTENT_COLLIDEALL\" ) = (int)FTENT_COLLIDEALL;" )
        mb.add_registration_code( "bp::scope().attr( \"FTENT_PERSIST\" ) = (int)FTENT_PERSIST;" )
        mb.add_registration_code( "bp::scope().attr( \"FTENT_COLLIDEKILL\" ) = (int)FTENT_COLLIDEKILL;" )
        mb.add_registration_code( "bp::scope().attr( \"FTENT_PLYRATTACHMENT\" ) = (int)FTENT_PLYRATTACHMENT;" )
        mb.add_registration_code( "bp::scope().attr( \"FTENT_SPRANIMATELOOP\" ) = (int)FTENT_SPRANIMATELOOP;" )
        mb.add_registration_code( "bp::scope().attr( \"FTENT_SMOKEGROWANDFADE\" ) = (int)FTENT_SMOKEGROWANDFADE;" )
        mb.add_registration_code( "bp::scope().attr( \"FTENT_ATTACHTOTARGET\" ) = (int)FTENT_ATTACHTOTARGET;" )
        mb.add_registration_code( "bp::scope().attr( \"FTENT_NOMODEL\" ) = (int)FTENT_NOMODEL;" )
        mb.add_registration_code( "bp::scope().attr( \"FTENT_CLIENTCUSTOM\" ) = (int)FTENT_CLIENTCUSTOM;" )
        mb.add_registration_code( "bp::scope().attr( \"FTENT_WINDBLOWN\" ) = (int)FTENT_WINDBLOWN;" )
        mb.add_registration_code( "bp::scope().attr( \"FTENT_NEVERDIE\" ) = (int)FTENT_NEVERDIE;" )
        mb.add_registration_code( "bp::scope().attr( \"FTENT_BEOCCLUDED\" ) = (int)FTENT_BEOCCLUDED;" )
        mb.add_registration_code( "bp::scope().attr( \"FTENT_CHANGERENDERONCOLLIDE\" ) = (int)FTENT_CHANGERENDERONCOLLIDE;" )
        mb.add_registration_code( "bp::scope().attr( \"FTENT_COLLISIONGROUP\" ) = (int)FTENT_COLLISIONGROUP;" )
        mb.add_registration_code( "bp::scope().attr( \"FTENT_ALIGNTOMOTION\" ) = (int)FTENT_ALIGNTOMOTION;" )
        mb.add_registration_code( "bp::scope().attr( \"FTENT_CLIENTSIDEPARTICLES\" ) = (int)FTENT_CLIENTSIDEPARTICLES;" )
        mb.add_registration_code( "bp::scope().attr( \"FTENT_USEFASTCOLLISIONS\" ) = (int)FTENT_USEFASTCOLLISIONS;" )
        
    def Parse(self, mb):
        # Exclude everything by default
        mb.decls().exclude() 
        
        mb.free_functions( 'DispatchEffect' ).include()
        
        # Data for dispatcheffect
        mb.class_('CEffectData').include()
        mb.class_('CEffectData').vars('m_vOrigin').rename('origin')
        mb.class_('CEffectData').vars('m_vStart').rename('start')
        mb.class_('CEffectData').vars('m_vNormal').rename('normal')
        mb.class_('CEffectData').vars('m_vAngles').rename('angles')
        mb.class_('CEffectData').vars('m_fFlags').rename('flags')
        mb.class_('CEffectData').vars('m_flScale').rename('scale')
        mb.class_('CEffectData').vars('m_flMagnitude').rename('magnitude')
        mb.class_('CEffectData').vars('m_flRadius').rename('radius')
        mb.class_('CEffectData').vars('m_nAttachmentIndex').rename('attachmentindex')
        mb.class_('CEffectData').vars('m_nSurfaceProp').rename('surfaceprop')
        mb.class_('CEffectData').vars('m_nMaterial').rename('material')
        mb.class_('CEffectData').vars('m_nDamageType').rename('damagetype')
        mb.class_('CEffectData').vars('m_nHitBox').rename('hitbox')
        mb.class_('CEffectData').vars('m_nColor').rename('color')
        
        # ITempEntsSystem
        cls = mb.class_('ITempEntsSystem')
        cls.include()
        cls.mem_funs().virtuality = 'not virtual' 

        mb.add_registration_code( 'bp::scope().attr( "te" ) = boost::ref(te);' )
         
        if self.isServer:
            self.ParseServer(mb)
        else:
            self.ParseClient(mb)
            
        # Disable shared warnings
        DisableKnownWarnings(mb)    