from srcpy.module_generators import ClientModuleGenerator

from pyplusplus import function_transformers as FT
from pyplusplus.module_builder import call_policies
from pygccxml.declarations import matchers

class VGUI(ClientModuleGenerator):
    module_name = '_vgui'
    
    files = [
        'cbase.h',
        
        'vgui_controls/Controls.h',
        'vgui/Cursor.h',
        
        'vgui_controls/Panel.h',
        'view_shared.h',
        'vgui/IBorder.h',
        'vgui_controls/AnimationController.h',
        
        'vgui/IInput.h',
        'vgui/ISystem.h',
        'vgui/IScheme.h',
        'vgui/ILocalize.h',
        'iclientmode.h',
        'vgui_controls/MessageMap.h',
        'ienginevgui.h',
        'hud.h',
        'hudelement.h',
        
        'srcpy_vgui.h',
        'wars_vgui_screen.h',
        
        'srcpy_hud.h',
    ]
    
    def ParseInterfaces(self, mb):
        # ISchemeManager
        mb.free_function('scheme').include()
        mb.free_function('scheme').call_policies = call_policies.return_value_policy(call_policies.reference_existing_object)
        
        cls = mb.class_('ISchemeManager')
        cls.include()
        cls.mem_funs().virtuality = 'not virtual' 
        cls.mem_funs( 'GetImage' ).call_policies = call_policies.return_value_policy(call_policies.reference_existing_object)
        cls.mem_funs( 'GetIScheme' ).call_policies = call_policies.return_value_policy(call_policies.reference_existing_object)
        #cls.mem_funs( 'GetBorder' ).call_policies = call_policies.return_value_policy(call_policies.reference_existing_object)
        
        if self.settings.branch == 'swarm':
            cls.mem_funs('GetSurface').exclude()
        
        # IScheme
        cls = mb.class_('IScheme')
        cls.include()
        cls.mem_funs().virtuality = 'not virtual' 
        cls.mem_funs('GetBorder').call_policies = call_policies.return_value_policy(call_policies.reference_existing_object)
        if self.settings.branch == 'source2013':
            cls.mem_funs('GetBorderAtIndex').call_policies = call_policies.return_value_policy(call_policies.reference_existing_object)

        if self.settings.branch == 'swarm':
            cls.class_('fontalias_t').exclude()
        
        # ILocalize
        cls = mb.class_('PyLocalize')
        cls.rename('Localize')
        cls.include()
        cls.mem_fun('Find').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        cls.mem_fun('GetValueByIndex').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        if self.settings.branch == 'swarm':
            mb.var('INVALID_STRING_INDEX').include()
        
        #mb.var('g_pylocalize').include()
        #mb.var('g_pylocalize').rename('localize')
        mb.add_registration_code( "bp::scope().attr( \"localize\" ) = boost::ref(g_pylocalize);" )
        
        # IPanel
        cls = mb.class_('CWrapIPanel')
        cls.include()
        cls.rename('IPanel')    
        cls.mem_funs( 'GetPos' ).add_transformation( FT.output('x'), FT.output('y') )
        cls.mem_funs( 'GetSize' ).add_transformation( FT.output('wide'), FT.output('tall') )
        cls.mem_funs( 'GetAbsPos' ).add_transformation( FT.output('x'), FT.output('y') )
        if self.settings.branch == 'swarm':
            cls.mem_funs( 'Plat' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
        cls.mem_funs( 'GetPanel' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
        
        mb.free_function('wrapipanel').include()
        mb.free_function( 'wrapipanel' ).call_policies = call_policies.return_value_policy(call_policies.reference_existing_object) 
        mb.free_function( 'wrapipanel' ).rename('ipanel')   

        # VGUI Input
        cls = mb.class_('IInput')
        cls.include()
        cls.mem_funs().virtuality = 'not virtual' 
        #cls.mem_funs('GetCursorPos').add_transformation( FT.output('x'), FT.output('y') )
        #cls.mem_funs('GetCursorPosition').add_transformation( FT.output('x'), FT.output('y') )
        #cls.mem_funs('GetIMEWindow').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        cls.mem_fun('GetIMEWindow').exclude()
        cls.mem_fun('SetIMEWindow').exclude()
        cls.mem_fun('GetIMEConversionModes').exclude()
        cls.mem_fun('GetIMELanguageList').exclude()
        cls.mem_fun('GetIMELanguageShortCode').exclude()
        cls.mem_fun('GetIMESentenceModes').exclude()
        
        cls.classes().exclude()
        
        mb.free_function('input').include()
        mb.free_function('input').call_policies = call_policies.return_value_policy(call_policies.reference_existing_object) 
        mb.free_function('input').rename('vgui_input')
        
        mb.free_function('PyInput_GetCursorPos').include()
        mb.free_function('PyInput_GetCursorPosition').include()
        
        # ISystem
        cls = mb.class_('ISystem')
        cls.include()
        cls.mem_funs().virtuality = 'not virtual' 
        cls.mem_funs('GetUserConfigFileData').call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        cls.mem_funs('GetRegistryInteger').add_transformation(FT.output('value'))
        
        # Exclude registry modification functions, likely not needed from Python
        cls.mem_funs('SetRegistryInteger').exclude()
        cls.mem_funs('SetRegistryString').exclude()
        cls.mem_funs('DeleteRegistryKey').exclude()
        
        cls.mem_fun('SetClipboardImage').exclude()
        
        # Properly wrap getting a registery string
        mb.add_declaration_code('''static boost::python::tuple GetRegistryString_cc10e70c5f6b49d5963b27442c970b19( ::vgui::ISystem & inst, char const * key ){
    char value2[512];
    bool result = inst.GetRegistryString(key, value2, sizeof(value2));
    return bp::make_tuple( result, value2 );
}
        ''')
        cls.mem_funs('GetRegistryString').exclude()
        cls.add_registration_code('''def( 
            "GetRegistryString"
            , (boost::python::tuple (*)( ::vgui::ISystem &,char const * ))( &GetRegistryString_cc10e70c5f6b49d5963b27442c970b19 )
            , ( bp::arg("inst"), bp::arg("key") ) )
        ''')
        
        vgui = mb.namespace('vgui')
        vgui.free_function('system').include()
        vgui.free_function('system').call_policies = call_policies.return_value_policy(call_policies.reference_existing_object) 
        vgui.free_function('system').rename('vgui_system')
        
        # IClientMode
        cls = mb.class_( 'IClientMode' )
        cls.include()
        cls.mem_funs().virtuality = 'not virtual' 
        
        if self.settings.branch == 'swarm':
            mb.add_declaration_code( "IClientMode *wrap_GetClientMode( void )\r\n{\r\n\treturn GetClientMode();\r\n}\r\n" )
            mb.add_registration_code( 'bp::def( "GetClientMode", wrap_GetClientMode, bp::return_value_policy<bp::reference_existing_object>() );' )
            
        if self.settings.branch == 'source2013':
            # TODO: Returns wchar_t *. Get a converter.
            cls.mem_fun('GetMapName').exclude()
            cls.mem_fun('GetServerName').exclude()
        
        cls.mem_funs( 'GetViewport' ).call_policies = call_policies.return_value_policy(call_policies.reference_existing_object)
        cls.mem_funs( 'GetViewportAnimationController' ).call_policies = call_policies.return_value_policy(call_policies.reference_existing_object)
        cls.mem_funs( 'GetMessagePanel' ).call_policies = call_policies.return_value_policy(call_policies.reference_existing_object)
        cls.mem_funs( 'AdjustEngineViewport' ).add_transformation( FT.output('x'), FT.output('y'), FT.output('width'), FT.output('height') )
        cls.mem_funs( 'ActivateInGameVGuiContext' ).include()  # Not safe, but IClientMode should not be overridden.
        
        if self.settings.branch == 'swarm':
            cls.mem_funs( 'GetPanelFromViewport' ).call_policies = call_policies.return_value_policy(call_policies.reference_existing_object)
        
    def ParseISurface(self, mb):
        cls = mb.class_('CWrapSurface')
        cls.include()
        cls.rename('ISurface')
        mb.free_function('wrapsurface').include()
        mb.free_function( 'wrapsurface' ).call_policies = call_policies.return_value_policy(call_policies.reference_existing_object) 
        mb.free_function( 'wrapsurface' ).rename('surface')    
        
        mb.enum('CursorCode').include()
        mb.enum('FontDrawType_t').include()
        if self.settings.branch == 'swarm':
            mb.class_('FontVertex_t').include()
        else:
            mb.class_('Vertex_t').include()
        mb.class_('IntRect').include()
        
        cls.mem_funs( 'DrawGetTextPos' ).add_transformation( FT.output('x'), FT.output('y') )
        cls.mem_funs( 'DrawGetTextureSize' ).add_transformation( FT.output('wide'), FT.output('tall') )
        cls.mem_funs( 'GetScreenSize' ).add_transformation( FT.output('wide'), FT.output('tall') )
        cls.mem_funs( 'GetCharABCwide' ).add_transformation( FT.output('a'), FT.output('b'), FT.output('c') )
        cls.mem_funs( 'GetWorkspaceBounds' ).add_transformation( FT.output('x'), FT.output('y'), FT.output('wide'), FT.output('tall') )
        cls.mem_funs( 'GetAbsoluteWindowBounds' ).add_transformation( FT.output('x'), FT.output('y'), FT.output('wide'), FT.output('tall') )
        cls.mem_funs( 'GetProportionalBase' ).add_transformation( FT.output('width'), FT.output('height') )
        cls.mem_funs( 'SurfaceGetCursorPos' ).add_transformation( FT.output('x'), FT.output('y') )
        
        cls.mem_funs( 'CreateHTMLWindow' ).call_policies = call_policies.return_value_policy(call_policies.reference_existing_object) 
        cls.mem_funs( 'DrawGetTextureMatInfoFactory' ).call_policies = call_policies.return_value_policy(call_policies.reference_existing_object) 
        cls.mem_funs( 'GetIconImageForFullPath' ).call_policies = call_policies.return_value_policy(call_policies.reference_existing_object) 
    
    def ParseHUD(self, mb):
        # CHud
        cls = mb.class_('CHud')
        cls.include()
        
        if self.settings.branch == 'swarm':
            mb.free_function('GetHud').include()
            mb.free_function('GetHud').call_policies = call_policies.return_value_policy(call_policies.reference_existing_object)
        else:
            mb.vars('gHUD').include()
        
        cls.mem_funs( 'FindElement' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
        if self.settings.branch != 'swarm': # ASW should use HudIcons() / CHudIcons
            #cls.mem_funs( 'GetIcon' ).call_policies = call_policies.return_value_policy( call_policies.manage_new_object )
            #cls.mem_funs( 'AddUnsearchableHudIconToList' ).call_policies = call_policies.return_value_policy( call_policies.manage_new_object ) 
            #cls.mem_funs( 'AddSearchableHudIconToList' ).call_policies = call_policies.return_value_policy( call_policies.manage_new_object )
            # The HUD only cleans up when you close the game, so internal references should't be a problem.
            cls.mem_funs( 'GetIcon' ).call_policies = call_policies.return_internal_reference()
            cls.mem_funs( 'AddUnsearchableHudIconToList' ).call_policies = call_policies.return_internal_reference()
            cls.mem_funs( 'AddSearchableHudIconToList' ).call_policies = call_policies.return_internal_reference()
        cls.vars('m_HudList').exclude()
        
        if self.settings.branch == 'swarm':
            cls.mem_funs('GetHudList').exclude()
            cls.mem_funs('GetHudPanelList').exclude()
            
        cls = mb.class_('CHudElement')
        cls.include()
        cls.mem_funs('GetName').exclude()
        cls.vars('m_pyInstance').exclude()
        cls.mem_funs('SetActive').virtuality = 'not virtual' 
        cls.mem_funs('IsActive').virtuality = 'not virtual'
        cls.mem_funs('ShouldDraw').virtuality = 'not virtual' # TODO: What if we need better control in Python?
        cls.mem_funs('ProcessInput').virtuality = 'not virtual' # TODO: Do we ever need this in Python?
        
        cls = mb.class_('CPyHudElementHelper')
        cls.include()
        cls.rename('CHudElementHelper')
        cls.mem_funs('GetNext').exclude()

        # HudIcons
        if self.settings.branch == 'swarm':
            cls = mb.class_('CHudIcons')
            cls.include()
            # FIXME
            cls.mem_funs( 'GetIcon' ).call_policies = call_policies.return_internal_reference()
            #cls.mem_funs('GetIcon').call_policies = call_policies.return_value_policy(call_policies.reference_existing_object)
            #cls.mem_funs('GetIcon' ).call_policies = call_policies.return_value_policy( call_policies.manage_new_object ) 
            cls.mem_funs('AddUnsearchableHudIconToList').exclude()
            cls.mem_funs('AddSearchableHudIconToList').exclude()
            #cls.mem_funs('AddUnsearchableHudIconToList').call_policies = call_policies.return_value_policy( call_policies.manage_new_object ) 
            #cls.mem_funs('AddSearchableHudIconToList').call_policies = call_policies.return_value_policy( call_policies.manage_new_object ) 
            mb.free_function('HudIcons').include()
            mb.free_function('HudIcons').call_policies = call_policies.return_value_policy(call_policies.reference_existing_object) 
            
        # CHudTexture
        mb.class_('CHudTexture').include()
        
    def ParseMisc(self, mb):
        # Include some empty classes
        #IncludeEmptyClass(mb, 'CViewSetup')    <- TODO: Needed?
        
        # Animation Controller
        mb.free_function('GetAnimationController').include()
        mb.free_function('GetAnimationController').call_policies = call_policies.return_value_policy(call_policies.reference_existing_object)
        
        # Screen
        cls = mb.class_('WarsVGUIScreen')
        cls.include()

        # IVGUI/Tick signals
        mb.add_declaration_code( "void wrap_AddTickSignal( VPANEL panel, int intervalMilliseconds = 0 )\r\n{\r\n\tvgui::ivgui()->AddTickSignal(panel, intervalMilliseconds);\r\n}\r\n" )
        mb.add_declaration_code( "void wrap_RemoveTickSignal( VPANEL panel)\r\n{\r\n\tvgui::ivgui()->RemoveTickSignal(panel);\r\n}\r\n" )
        mb.add_registration_code( 'bp::def( "AddTickSignal", wrap_AddTickSignal, (bp::arg("panel"), bp::arg("intervalMilliseconds")=0 ) );' )
        mb.add_registration_code( 'bp::def( "RemoveTickSignal", wrap_RemoveTickSignal, bp::arg("panel") );' )
        
        # Gameui open?
        mb.free_function('PyIsGameUIVisible').include()
        mb.free_function('PyIsGameUIVisible').rename('IsGameUIVisible')
        mb.free_function('PyGetPanel').include()
        mb.free_function('PyGetPanel').rename('GetPanel')
        #mb.free_function('PyGetPanel').call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
        mb.enum('VGuiPanel_t').include()
        
        # Message map types
        mb.enum('DataType_t').include()

        # Some defines
        mb.add_registration_code( "bp::scope().attr( \"INVALID_FONT\" ) = (int)0;" )
        mb.add_registration_code( "bp::scope().attr( \"INVALID_PANEL\" ) = (int)0xffffffff;" )
        
    def Parse(self, mb):
        # Exclude everything by default
        mb.decls().exclude()  

        self.ParseInterfaces(mb)
        self.ParseISurface(mb)
        self.ParseHUD(mb)
        self.ParseMisc(mb)
        
        # Remove any protected function 
        mb.calldefs( matchers.access_type_matcher_t( 'protected' ) ).exclude()
        
        self.ApplyCommonRules(mb)
        
        