import re

from srcpy.module_generators import SemiSharedModuleGenerator
from srcpy.matchers import calldef_withtypes

from pyplusplus import function_transformers as FT
from pyplusplus.module_builder import call_policies
from pygccxml.declarations import matchers, pointer_t, const_t, declarated_t, char_t

# Templates for the most common callback cases
callback_wrapper_tmpl = '''struct %(name)sCallback_wrapper : %(name)sCallback, bp::wrapper< %(name)sCallback > {

    %(name)sCallback_wrapper(%(name)sCallback const & arg )
    : %(name)sCallback( arg )
      , bp::wrapper< %(name)sCallback >(){
        // copy constructor
        
    }

    %(name)sCallback_wrapper()
    : %(name)sCallback()
      , bp::wrapper< %(name)sCallback >(){
        // constructor
    
    }

    virtual void On%(name)s( ::%(dataclass)s * pData ) {
        PY_OVERRIDE_CHECK( %(name)sCallback, On%(name)s )
        PY_OVERRIDE_LOG( _steam, %(name)sCallback, On%(name)s )
        bp::override func_On%(name)s = this->get_override( "On%(name)s" );
        if( func_On%(name)s.ptr() != Py_None )
            try {
                func_On%(name)s( boost::python::ptr(pData) );
            } catch(bp::error_already_set &) {
                PyErr_Print();
                this->%(name)sCallback::On%(name)s( pData );
            }
        else
            this->%(name)sCallback::On%(name)s( pData );
    }
    
    void default_On%(name)s( ::%(dataclass)s * pData ) {
        %(name)sCallback::On%(name)s( pData );
    }
};
'''

callback_reg_tmpl = '''{ //::%(name)sCallback
        typedef bp::class_< %(name)sCallback_wrapper > %(name)sCallback_exposer_t;
        %(name)sCallback_exposer_t %(name)sCallback_exposer = %(name)sCallback_exposer_t( "%(name)sCallback", bp::init<>() );
        bp::scope %(name)sCallback_scope( %(name)sCallback_exposer );
        { //::%(name)sCallback::On%(name)s
        
            typedef void ( ::%(name)sCallback::*On%(name)s_function_type )( ::%(dataclass)s * ) ;
            typedef void ( %(name)sCallback_wrapper::*default_On%(name)s_function_type )( ::%(dataclass)s * ) ;
            
            %(name)sCallback_exposer.def( 
                "On%(name)s"
                , On%(name)s_function_type(&::%(name)sCallback::On%(name)s)
                , default_On%(name)s_function_type(&%(name)sCallback_wrapper::default_On%(name)s)
                , ( bp::arg("data") ) );
        
        }
    }
'''

callresult_wrapper_tmpl = '''struct %(name)sCallResult_wrapper : %(name)sCallResult, bp::wrapper< %(name)sCallResult > {

    %(name)sCallResult_wrapper(%(name)sCallResult const & arg )
    : %(name)sCallResult( arg )
      , bp::wrapper< %(name)sCallResult >(){
        // copy constructor
    }

    %(name)sCallResult_wrapper(::SteamAPICall_t steamapicall )
    : %(name)sCallResult( steamapicall )
      , bp::wrapper< %(name)sCallResult >(){
        // constructor
    }

    virtual void On%(name)s( ::%(dataclass)s * pData, bool bIOFailure ) {
        PY_OVERRIDE_CHECK( %(name)sCallResult, On%(name)s )
        PY_OVERRIDE_LOG( _steam, %(name)sCallResult, On%(name)s )
        bp::override func_On%(name)s = this->get_override( "On%(name)s" );
        if( func_On%(name)s.ptr() != Py_None )
            try {
                func_On%(name)s( boost::python::ptr(pData), bIOFailure );
            } catch(bp::error_already_set &) {
                PyErr_Print();
                this->%(name)sCallResult::On%(name)s( pData, bIOFailure );
            }
        else
            this->%(name)sCallResult::On%(name)s( pData, bIOFailure );
    }
    
    void default_On%(name)s( ::%(dataclass)s * pData, bool bIOFailure ) {
        %(name)sCallResult::On%(name)s( pData, bIOFailure );
    }
};
'''

callresult_reg_tmpl = '''{ //::%(name)sCallResult
        typedef bp::class_< %(name)sCallResult_wrapper > %(name)sCallResult_exposer_t;
        %(name)sCallResult_exposer_t %(name)sCallResult_exposer = %(name)sCallResult_exposer_t( "%(name)sCallResult", bp::init< SteamAPICall_t >(( bp::arg("steamapicall") )) );
        bp::scope %(name)sCallResult_scope( %(name)sCallResult_exposer );
        bp::implicitly_convertible< SteamAPICall_t, %(name)sCallResult >();
        { //::%(name)sCallResult::On%(name)s
        
            typedef void ( ::%(name)sCallResult::*On%(name)s_function_type )( ::%(dataclass)s *,bool ) ;
            typedef void ( %(name)sCallResult_wrapper::*default_On%(name)s_function_type )( ::%(dataclass)s *,bool ) ;
            
            %(name)sCallResult_exposer.def( 
                "On%(name)s"
                , On%(name)s_function_type(&::%(name)sCallResult::On%(name)s)
                , default_On%(name)s_function_type(&%(name)sCallResult_wrapper::default_On%(name)s)
                , ( bp::arg("data"), bp::arg("iofailure") ) );
        
        }
    }
'''

class Steam(SemiSharedModuleGenerator):
    module_name = '_steam'
    steamsdkversion = (1, 15)
    
    @property
    def files(self):
        files = [
            'cbase.h',
            'steam/steam_api.h',
            'steam/isteamfriends.h',
            'steam/isteamutils.h',
            'steam/isteamuser.h',
            'steam/steamclientpublic.h',
            'steam/isteamuserstats.h',
            
            'srcpy_steam.h',
        ]
        return files
        
    def PythonfyVariables(self, cls):
        ''' Removes prefixes from variable names and lower cases the variable. '''
        for var in cls.vars(allow_empty=True):
            varname = var.name
            varname = re.sub('^(m_ul|m_un|m_us|m_n|m_e|m_E|m_i|m_b|m_c|m_rgf|m_sz)', '', varname)
            varname = re.sub('^(m_)', '', varname)
            varname = varname.lower()
            var.rename(varname)
            
    def AddSteamCallback(self, name, dataclsname):
        mb = self.mb
        
        # Include the dataclass
        cls = mb.class_(dataclsname)
        cls.include()
        self.PythonfyVariables(cls)
        
        # Generate the wrapper callback
        mb.add_declaration_code('PY_STEAM_CALLBACK_WRAPPER( %s, %s );' % (name, dataclsname))
        mb.add_declaration_code(callback_wrapper_tmpl % {'name' : name, 'dataclass' : dataclsname})
        mb.add_registration_code(callback_reg_tmpl % {'name' : name, 'dataclass' : dataclsname})
        
    def AddSteamCallResult(self, name, dataclsname):
        mb = self.mb
        
        # Include the dataclass
        cls = mb.class_(dataclsname)
        cls.include()
        self.PythonfyVariables(cls)
        
        # Generate the wrapper callback
        mb.add_declaration_code('PY_STEAM_CALLRESULT_WRAPPER( %s, %s );' % (name, dataclsname))
        mb.add_declaration_code(callresult_wrapper_tmpl % {'name' : name, 'dataclass' : dataclsname})
        mb.add_registration_code(callresult_reg_tmpl % {'name' : name, 'dataclass' : dataclsname})
        
    def ParseSteamFriends(self, mb):
        cls = mb.class_('ISteamFriends')
        cls.include()
        cls.mem_funs().virtuality = 'not virtual'
        cls.mem_fun('GetFriendGamePlayed').exclude()
        
        mb.enum('EFriendRelationship').include()
        mb.enum('EPersonaState').include()
        mb.enum('EPersonaChange').include()
        mb.add_registration_code( "bp::scope().attr( \"k_cchPersonaNameMax\" ) = (int)k_cchPersonaNameMax;" )
        
        self.AddSteamCallback('PersonaStateChange', 'PersonaStateChange_t')
        self.AddSteamCallback('GameOverlayActivated', 'GameOverlayActivated_t')
        self.AddSteamCallback('GameServerChangeRequested', 'GameServerChangeRequested_t')
        self.AddSteamCallback('GameLobbyJoinRequested', 'GameLobbyJoinRequested_t')
        self.AddSteamCallback('AvatarImageLoaded', 'AvatarImageLoaded_t')
        self.AddSteamCallback('ClanOfficerListResponse', 'ClanOfficerListResponse_t')
        self.AddSteamCallback('FriendRichPresenceUpdate', 'FriendRichPresenceUpdate_t')
        self.AddSteamCallback('GameRichPresenceJoinRequested', 'GameRichPresenceJoinRequested_t')
        self.AddSteamCallback('GameConnectedClanChatMsg', 'GameConnectedClanChatMsg_t')
        self.AddSteamCallback('GameConnectedChatJoin', 'GameConnectedChatJoin_t')
        self.AddSteamCallback('GameConnectedChatLeave', 'GameConnectedChatLeave_t')
        self.AddSteamCallback('DownloadClanActivityCountsResult', 'DownloadClanActivityCountsResult_t')
        self.AddSteamCallback('JoinClanChatRoomCompletionResult', 'JoinClanChatRoomCompletionResult_t')
        self.AddSteamCallback('GameConnectedFriendChatMsg', 'GameConnectedFriendChatMsg_t')
        self.AddSteamCallback('FriendsGetFollowerCount', 'FriendsGetFollowerCount_t')
        self.AddSteamCallback('FriendsIsFollowing', 'FriendsIsFollowing_t')
        self.AddSteamCallback('FriendsEnumerateFollowingList', 'FriendsEnumerateFollowingList_t')
        self.AddSteamCallback('SetPersonaNameResponse', 'SetPersonaNameResponse_t')
        
    def ParseMatchmaking(self, mb):
        # The main matchmaking interface
        cls = mb.class_('ISteamMatchmaking')
        cls.include()
        cls.mem_funs().virtuality = 'not virtual'
        cls.mem_funs('GetLobbyGameServer').add_transformation(FT.output('punGameServerIP'), FT.output('punGameServerPort'), FT.output('psteamIDGameServer'))
        
        mb.free_function('PyGetLobbyDataByIndex').include()
        mb.free_function('PySendLobbyChatMsg').include()
        mb.free_function('PyGetLobbyChatEntry').include()
        
        mb.var('k_uAPICallInvalid').include()
        
        self.AddSteamCallResult('LobbyMatchList', 'LobbyMatchList_t')
        self.AddSteamCallResult('LobbyGameCreated', 'LobbyGameCreated_t')
        self.AddSteamCallResult('LobbyCreated', 'LobbyCreated_t')
        self.AddSteamCallResult('LobbyEnter', 'LobbyEnter_t')
        
        self.AddSteamCallback('LobbyChatUpdate', 'LobbyChatUpdate_t')
        self.AddSteamCallback('LobbyChatMsg', 'LobbyChatMsg_t')
        self.AddSteamCallback('LobbyDataUpdate', 'LobbyDataUpdate_t')
        
        # Servers matchmaking interface
        cls = mb.class_('PySteamMatchmakingServers')
        cls.include()
        cls.rename('SteamMatchmakingServers')
        
        cls = mb.class_('PySteamMatchmakingServerListResponse')
        cls.include()
        cls.rename('SteamMatchmakingServerListResponse')
        cls.mem_fun('PyServerResponded').rename('ServerResponded')
        cls.mem_fun('PyServerFailedToRespond').rename('ServerFailedToRespond')
        cls.mem_fun('PyRefreshComplete').rename('RefreshComplete')
        
        cls = mb.class_('gameserveritem_t')
        cls.include()
        cls.mem_fun('SetName').exclude()
        self.PythonfyVariables(cls)
        cls.var('m_szGameDir').exclude()
        cls.var('m_szMap').exclude()
        cls.var('m_szGameDescription').exclude()
        cls.var('m_szGameTags').exclude()
        
        cls = mb.class_('pygameserveritem_t')
        cls.include()
        self.PythonfyVariables(cls)
        self.AddProperty(cls, 'gamedir', 'GetGameDir')
        self.AddProperty(cls, 'map', 'GetMap')
        self.AddProperty(cls, 'gamedescription', 'GetGameDescription')
        self.AddProperty(cls, 'gametags', 'GetGameTags')
        
        cls = mb.class_('servernetadr_t')
        cls.include()
        cls.rename('servernetadr')
        
        cls = mb.class_('PySteamMatchmakingPingResponse')
        cls.include()
        cls.rename('SteamMatchmakingPingResponse')
        
        cls = mb.class_('PySteamMatchmakingPlayersResponse')
        cls.include()
        cls.rename('SteamMatchmakingPlayersResponse')
        
        cls = mb.class_('PySteamMatchmakingRulesResponse')
        cls.include()
        cls.rename('SteamMatchmakingRulesResponse')
        
        # Enums
        mb.enums('ELobbyType').include()
        mb.enums('ELobbyComparison').include()
        mb.enums('ELobbyDistanceFilter').include()
        mb.enums('EMatchMakingServerResponse').include()
        
    def ParseUserStats(self, mb):
        cls = mb.class_('ISteamUserStats')
        cls.include()
        cls.mem_funs().virtuality = 'not virtual'
        
        self.AddSteamCallResult('NumberOfCurrentPlayers', 'NumberOfCurrentPlayers_t')
        
        mb.free_function('PyGetStatFloat').include()
        mb.free_function('PyGetStatInt').include()
        
    def ParseGameServer(self, mb):
        cls = mb.class_('ISteamGameServer')
        cls.include()
        cls.mem_funs().virtuality = 'not virtual'
        
    def ParseServerOnly(self, mb):
        # Accessor class game server
        mb.add_registration_code( "bp::scope().attr( \"steamgameserverapicontext\" ) = boost::ref(steamgameserverapicontext);" )
        cls = mb.class_('CSteamGameServerAPIContext')
        cls.include()
        cls.mem_fun('Init').exclude()
        cls.mem_fun('Clear').exclude()
        cls.mem_fun('SteamGameServerUtils').exclude()
        cls.mem_fun('SteamGameServerNetworking').exclude()
        cls.mem_fun('SteamGameServerStats').exclude()
        
        cls.mem_funs('SteamGameServer').call_policies = call_policies.return_internal_reference()
    
        self.ParseGameServer(mb)

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
        cls.mem_funs('SetFromString').exclude() # No definition...
        cls.mem_funs('SetFromSteam2String').exclude() # No definition...
        cls.mem_funs('BValidExternalSteamID').exclude() # No definition...
        
        mb.enum('EResult').include()
        mb.enum('EDenyReason').include()
        mb.enum('EUniverse').include()
        mb.enum('EAccountType').include()
        mb.enum('ESteamUserStatType').include()
        mb.enum('EChatEntryType').include()
        mb.enum('EChatRoomEnterResponse').include()
        mb.enum('EChatMemberStateChange').include()
        
        # Generic API functions
        mb.free_function('SteamAPI_RunCallbacks').include()
        
        # Accessor class client
        mb.add_registration_code( "bp::scope().attr( \"steamapicontext\" ) = boost::ref(steamapicontext);" )
        cls = mb.class_('CSteamAPIContext')
        cls.include()
        cls.mem_fun('Init').exclude()
        cls.mem_fun('Clear').exclude()
        cls.mem_fun('SteamApps').exclude()
        
        if self.steamsdkversion > (1, 11):
            cls.mem_fun('SteamHTTP').exclude()
        if self.steamsdkversion > (1, 15):
            cls.mem_fun('SteamScreenshots').exclude()
        if self.steamsdkversion > (1, 20):
            cls.mem_fun('SteamUnifiedMessages').exclude()
        cls.mem_fun('SteamMatchmakingServers').exclude() # Full python class wrapper

        cls.mem_fun('SteamNetworking').exclude()
        cls.mem_fun('SteamRemoteStorage').exclude()
        if self.steamsdkversion > (1, 16):
            cls.mem_fun('SteamAppList').exclude()
            cls.mem_fun('SteamController').exclude()
            cls.mem_fun('SteamMusic').exclude()
            cls.mem_fun('SteamMusicRemote').exclude()
            cls.mem_fun('SteamUGC').exclude() 
            
        cls.mem_funs('SteamFriends').call_policies = call_policies.return_internal_reference()
        cls.mem_funs('SteamUtils').call_policies = call_policies.return_internal_reference()
        cls.mem_funs('SteamMatchmaking').call_policies = call_policies.return_internal_reference()
        cls.mem_funs('SteamMatchmakingServers').call_policies = call_policies.return_internal_reference()
        cls.mem_funs('SteamUser').call_policies = call_policies.return_internal_reference()
        cls.mem_funs('SteamUserStats').call_policies = call_policies.return_internal_reference()
        
        mb.add_registration_code( "bp::scope().attr( \"QUERY_PORT_NOT_INITIALIZED\" ) = (int)QUERY_PORT_NOT_INITIALIZED;" )
        mb.add_registration_code( "bp::scope().attr( \"QUERY_PORT_ERROR\" ) = (int)QUERY_PORT_ERROR;" )
        
        self.ParseSteamFriends(mb)
        
        # User
        cls = mb.class_('ISteamUser')
        cls.include()
        cls.mem_funs().virtuality = 'not virtual'
        
        # Utils
        cls = mb.class_('ISteamUtils')
        cls.include()
        cls.mem_funs().virtuality = 'not virtual'
        cls.mem_fun('GetImageRGBA').exclude()
        cls.mem_fun('GetImageSize').exclude()
        
        self.ParseMatchmaking(mb)
        self.ParseUserStats(mb)
        
        #mb.class_('ISteamUtils').mem_funs('GetImageSize').add_transformation( FT.output('pnWidth'), FT.output('pnHeight'))
        #mb.class_('ISteamUtils').mem_funs('GetCSERIPPort').add_transformation( FT.output('unIP'), FT.output('usPort'))
        
        if self.isserver:
            self.ParseServerOnly(mb)
        