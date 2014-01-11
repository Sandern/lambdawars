import re

from srcpy.module_generators import SemiSharedModuleGenerator

from pyplusplus import function_transformers as FT
from pyplusplus.module_builder import call_policies
from pygccxml.declarations import matchers, pointer_t, const_t, declarated_t, char_t

callback_wrapper_tmpl = '''struct %(name)sCallback_wrapper : %(name)sCallback, bp::wrapper< %(name)sCallback > {

    %(name)sCallback_wrapper(%(name)sCallback const & arg )
    : %(name)sCallback( arg )
      , bp::wrapper< %(name)sCallback >(){
        // copy constructor
        
    }

    %(name)sCallback_wrapper(::SteamAPICall_t steamapicall )
    : %(name)sCallback( steamapicall )
      , bp::wrapper< %(name)sCallback >(){
        // constructor
    
    }

    virtual void On%(name)s( ::%(dataclass)s * pData, bool bIOFailure ) {
        PY_OVERRIDE_CHECK( %(name)sCallback, On%(name)s )
        PY_OVERRIDE_LOG( _steam, %(name)sCallback, On%(name)s )
        bp::override func_On%(name)s = this->get_override( "On%(name)s" );
        if( func_On%(name)s.ptr() != Py_None )
            try {
                func_On%(name)s( boost::python::ptr(pData), bIOFailure );
            } catch(bp::error_already_set &) {
                PyErr_Print();
                this->%(name)sCallback::On%(name)s( pData, bIOFailure );
            }
        else
            this->%(name)sCallback::On%(name)s( pData, bIOFailure );
    }
    
    void default_On%(name)s( ::%(dataclass)s * pData, bool bIOFailure ) {
        %(name)sCallback::On%(name)s( pData, bIOFailure );
    }
};
'''

callback_reg_tmpl = '''{ //::%(name)sCallback
        typedef bp::class_< %(name)sCallback_wrapper > %(name)sCallback_exposer_t;
        %(name)sCallback_exposer_t %(name)sCallback_exposer = %(name)sCallback_exposer_t( "%(name)sCallback", bp::init< SteamAPICall_t >(( bp::arg("steamapicall") )) );
        bp::scope %(name)sCallback_scope( %(name)sCallback_exposer );
        bp::implicitly_convertible< SteamAPICall_t, %(name)sCallback >();
        { //::%(name)sCallback::On%(name)s
        
            typedef void ( ::%(name)sCallback::*On%(name)s_function_type )( ::%(dataclass)s *,bool ) ;
            typedef void ( %(name)sCallback_wrapper::*default_On%(name)s_function_type )( ::%(dataclass)s *,bool ) ;
            
            %(name)sCallback_exposer.def( 
                "On%(name)s"
                , On%(name)s_function_type(&::%(name)sCallback::On%(name)s)
                , default_On%(name)s_function_type(&%(name)sCallback_wrapper::default_On%(name)s)
                , ( bp::arg("data"), bp::arg("iofailure") ) );
        
        }
    }
'''

class Steam(SemiSharedModuleGenerator):
    module_name = '_steam'
    
    @property
    def files(self):
        files = [
            'cbase.h',
            'steam/steam_api.h',
            'steam/isteamfriends.h',
            'steam/isteamutils.h',
            'steam/steamclientpublic.h',
            
            'srcpy_steam.h',
        ]
        if self.settings.branch == 'swarm':
            files.append('$vgui_avatarimage.h')
        return files
        
    def PythonfyVariables(self, cls):
        ''' Removes prefixes from variable names and lower cases the variable. '''
        for var in cls.vars():
            varname = var.name
            varname = re.sub('^(m_ul|m_un|m_us|m_n|m_e)', '', varname)
            varname = varname.lower()
            var.rename(varname)
        
    def AddSteamCallback(self, name, dataclsname):
        mb = self.mb
        
        callbackname = '%sCallback' % (name)
        
        # Include the dataclass
        cls = mb.class_(dataclsname)
        cls.include()
        self.PythonfyVariables(cls)
        
        # Generate the wrapper callback
        mb.add_declaration_code('PY_STEAM_CALLBACK_WRAPPER( %s, %s );' % (name, dataclsname))
        
        mb.add_declaration_code(callback_wrapper_tmpl % {'name' : name, 'dataclass' : dataclsname} )
        
        mb.add_registration_code( callback_reg_tmpl % {'name' : name, 'dataclass' : dataclsname} )
        
    def ParseMatchmaking(self, mb):
        cls = mb.class_('ISteamMatchmaking')
        cls.include()
        cls.mem_funs().virtuality = 'not virtual'
        
        mb.free_function('PyGetLobbyDataByIndex').include()
        
        self.AddSteamCallback('LobbyMatchList', 'LobbyMatchList_t')
        self.AddSteamCallback('LobbyGameCreated', 'LobbyGameCreated_t')
        self.AddSteamCallback('LobbyCreated', 'LobbyCreated_t')
         
        # Enums
        mb.enums('ELobbyType').include()
        mb.enums('ELobbyComparison').include()
        mb.enums('ELobbyDistanceFilter').include()

    def Parse(self, mb):
        # Exclude everything by default
        mb.decls().exclude()  

        # Generic steam api call return result
        mb.typedef('SteamAPICall_t').include()

        # CSteamID
        cls = mb.class_('CSteamID')
        cls.include()
        constpchararg = pointer_t(const_t(declarated_t(char_t())))
        cls.constructors(matchers.calldef_matcher_t(arg_types=[constpchararg, None])).exclude()
        cls.mem_funs('Render').exclude()
        cls.mem_funs('SetFromStringStrict').exclude()
        cls.mem_funs('SetFromString').exclude()      # No definition...
        cls.mem_funs('SetFromSteam2String').exclude()      # No definition...
        cls.mem_funs('BValidExternalSteamID').exclude()      # No definition...
        
        mb.enum('EResult').include()
        mb.enum('EDenyReason').include()
        mb.enum('EUniverse').include()
        mb.enum('EAccountType').include()
        mb.enum('ESteamUserStatType').include()
        mb.enum('EChatEntryType').include()
        mb.enum('EChatRoomEnterResponse').include()
        
        # Accessor class for all
        mb.add_registration_code( "bp::scope().attr( \"steamapicontext\" ) = boost::ref(steamapicontext);" )
        cls = mb.class_('CSteamAPIContext')
        cls.include()
        cls.mem_fun('Init').exclude()
        cls.mem_fun('Clear').exclude()
        cls.mem_fun('SteamUser').exclude()
        cls.mem_fun('SteamUserStats').exclude()
        cls.mem_fun('SteamApps').exclude()
        cls.mem_fun('SteamMatchmakingServers').exclude()
        
        cls.mem_fun('SteamHTTP').exclude()
        cls.mem_fun('SteamScreenshots').exclude()
        cls.mem_fun('SteamUnifiedMessages').exclude()

        cls.mem_fun('SteamNetworking').exclude()
        cls.mem_fun('SteamRemoteStorage').exclude()
        
        cls.mem_funs('SteamFriends').call_policies = call_policies.return_internal_reference() 
        cls.mem_funs('SteamUtils').call_policies = call_policies.return_internal_reference() 
        cls.mem_funs('SteamMatchmaking').call_policies = call_policies.return_internal_reference() 
        
        mb.add_registration_code( "bp::scope().attr( \"QUERY_PORT_NOT_INITIALIZED\" ) = (int)QUERY_PORT_NOT_INITIALIZED;" )
        mb.add_registration_code( "bp::scope().attr( \"QUERY_PORT_ERROR\" ) = (int)QUERY_PORT_ERROR;" )
        
        # Friends
        cls = mb.class_('ISteamFriends')
        cls.include()
        cls.mem_funs().virtuality = 'not virtual'
        cls.mem_fun('GetFriendGamePlayed').exclude()
        
        mb.enum('EFriendRelationship').include()
        mb.enum('EPersonaState').include()
        mb.add_registration_code( "bp::scope().attr( \"k_cchPersonaNameMax\" ) = (int)k_cchPersonaNameMax;" )
        
        # Utils
        cls = mb.class_('ISteamUtils')
        cls.include()
        cls.mem_funs().virtuality = 'not virtual'
        cls.mem_fun('GetImageRGBA').exclude()
        cls.mem_fun('GetImageSize').exclude()
        
        self.ParseMatchmaking(mb)
        
        #mb.class_('ISteamUtils').mem_funs('GetImageSize').add_transformation( FT.output('pnWidth'), FT.output('pnHeight'))
        #mb.class_('ISteamUtils').mem_funs('GetCSERIPPort').add_transformation( FT.output('unIP'), FT.output('usPort'))
        