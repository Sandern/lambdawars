from srcpy.module_generators import ClientModuleGenerator
from srcpy.matchers import calldef_withtypes, MatcherTestInheritClass

from pygccxml.declarations import matcher, matchers, pointer_t, const_t, reference_t, declarated_t, char_t, int_t, wchar_t
from pyplusplus.code_creators import calldef
from pyplusplus import function_transformers as FT
from pyplusplus.module_builder import call_policies

# Convert templates for without vgui lib
ptr_convert_to_py_name_template_novguilib = '''struct %(ptr_convert_to_py_name)s : bp::to_python_converter<%(clsname)s *, ptr_%(clsname)s_to_handle>
{
    static PyObject* convert( %(clsname)s *s )
    {
        if( s ) 
        {
            PyObject *pObject = GetPyPanel( s );
            if( pObject )
                return bp::incref( pObject );
            else
                return bp::incref( bp::object( %(handlename)s(s) ).ptr() );
        }
        else
        {
            return bp::incref( Py_None );
        }
    }
};'''

convert_to_py_name_template_novguilib = '''struct %(convert_to_py_name)s : bp::to_python_converter<%(clsname)s, %(clsname)s_to_handle>
{
    static PyObject* convert( const %(clsname)s &s )
    {
        PyObject *pObject = GetPyPanel( &s );
        if( pObject )
            return bp::incref( pObject );
        else
            return bp::incref( bp::object( %(handlename)s( &s ) ).ptr() );
    }
};'''

# Converters when having the vgui lib
ptr_convert_to_py_name_template = '''struct %(ptr_convert_to_py_name)s : bp::to_python_converter<%(clsname)s *, ptr_%(clsname)s_to_handle>
{
    static PyObject* convert(%(clsname)s *s)
    {
        if( s )
        {
            if( s->GetPySelf() != NULL )
                return bp::incref( s->GetPySelf() ); 
            else
                return bp::incref( bp::object( %(handlename)s( s ) ).ptr() );
        }
        else
        {
            return bp::incref(Py_None);
        }
    }
};'''

convert_to_py_name_template = '''struct %(convert_to_py_name)s : bp::to_python_converter<%(clsname)s, %(clsname)s_to_handle>
{
    static PyObject* convert(const %(clsname)s &s)
    {
        if( s.GetPySelf() != NULL )
            return bp::incref( s.GetPySelf() );
        else
            return bp::incref( bp::object( %(handlename)s( (%(clsname)s *)&s) ).ptr() );
    }
};'''

convert_from_py_name_template = '''struct %(convert_from_py_name)s
{
    handle_to_%(clsname)s()
    {
        bp::converter::registry::insert(
            &extract_%(clsname)s,
            bp::type_id< %(clsname)s >()
            );
    }
    
    static void* extract_%(clsname)s(PyObject* op)
    {
        PyBaseVGUIHandle &h = bp::extract< PyBaseVGUIHandle & >(op);
        return h.Get();
    }
};'''

vguihandle_template = '''{ //::%(handlename)s
        typedef bp::class_< %(handlename)s, bp::bases<PyBaseVGUIHandle> > %(handlename)s_exposer_t;
        %(handlename)s_exposer_t %(handlename)s_exposer = %(handlename)s_exposer_t( "%(handlename)s", bp::init< >() );
        %(handlename)s_exposer.def( bp::init<  %(clsname)s * >(( bp::arg("val") )) );
        { //::%(handlename)s::GetAttr
            typedef bp::object ( ::%(handlename)s::*GetAttr_function_type )( const char * ) const;
            %(handlename)s_exposer.def( 
                "__getattr__"
                , GetAttr_function_type( &::%(handlename)s::GetAttr )
            );
        }
    }
'''

class VGUIControls(ClientModuleGenerator):
    module_name = '_vguicontrols'
    split = True
    
    files = [
        'cbase.h',
        
        'vgui_controls/Panel.h',
        'vgui_controls/AnimationController.h',
        'vgui_controls/EditablePanel.h',
        'vgui_controls/AnalogBar.h',
        'vgui_controls/Image.h',
        'vgui_controls/TextImage.h',
        'vgui_controls/ScrollBar.h',
        'vgui_controls/ScrollBarSlider.h',
        'vgui_controls/Menu.h',
        'vgui_controls/MenuButton.h',
        'vgui_controls/Frame.h',
        'vgui_controls/TextEntry.h',
        'vgui_controls/RichText.h',
        'vgui_controls/Tooltip.h',
        
        'vgui/IBorder.h',

        'vgui_bitmapimage.h',
        'vgui_avatarimage.h',
        
        'srcpy_vgui.h',
        'hl2wars_baseminimap.h',
        'vgui_video_general.h',
        
        'vgui/wars_model_panel.h',
    ]
    
    panel_cls_list = [  
        'AnimationController', 
        'Panel', 
        'EditablePanel', 
        'Frame', 
        'TextEntry', 
        'ScrollBar', 
        'ScrollBarSlider',
        'RichText',
        'CBaseMinimap',
        'VideoGeneralPanel',
        'CWars_Model_Panel',
    ]

    # Almost the same as above, maybe make a shared function?   
    def AddVGUIConverter(self, mb, clsname, novguilib, containsabstract=False):
        cls = mb.class_(clsname)
        
        handlename = clsname+'HANDLE'
        
        ptr_convert_to_py_name = 'ptr_%(clsname)s_to_handle' % {'clsname' : clsname}
        convert_to_py_name = '%(clsname)s_to_handle' % {'clsname' : clsname}
        convert_from_py_name = 'handle_to_%(clsname)s' % {'clsname' : clsname}
        
        # Arguments for templates
        strargs = {
            'clsname' : clsname,
            'handlename' : handlename,
            'ptr_convert_to_py_name' : ptr_convert_to_py_name,
            'convert_to_py_name' : convert_to_py_name,
            'convert_from_py_name' : convert_from_py_name,
        }
        
        # Add GetPySelf method
        cls.add_wrapper_code(
            'virtual PyObject *GetPySelf() const { return bp::detail::wrapper_base_::get_owner(*this); }'
        )
        cls.add_wrapper_code(
            'virtual bool IsPythonManaged() { return true; }'
        )
        
        constructors = cls.constructors(name=clsname)
        constructors.body = '\tg_PythonPanelCount++;'  
            
        cls.add_wrapper_code( '~%(clsname)s_wrapper( void ) { g_PythonPanelCount--; /*::PyDeletePanel( this, this );*/ }' % strargs ) 
        
        if novguilib:
            cls.add_wrapper_code( 'void DeletePanel( void ) { ::PyDeletePanel( this, this ); }' )
        
        # Add handle typedef
        mb.add_declaration_code( 'typedef PyVGUIHandle<%(clsname)s> %(handlename)s;\r\n' % strargs )
        
        # Expose handle code
        mb.add_registration_code(vguihandle_template % {'clsname' : clsname, 'handlename' : handlename}, True)
        
        # Add to python converters
        if novguilib:
            mb.add_declaration_code(ptr_convert_to_py_name_template_novguilib % strargs)
            
            if not containsabstract:
                mb.add_declaration_code(convert_to_py_name_template_novguilib % strargs)
        else:
            mb.add_declaration_code(ptr_convert_to_py_name_template % strargs)
            
            if not containsabstract:
                mb.add_declaration_code(convert_to_py_name_template % strargs)

        # Add from python converter
        mb.add_declaration_code(convert_from_py_name_template % strargs)
        
        # Add registration code
        mb.add_registration_code( ptr_convert_to_py_name+"();" )
        if not containsabstract:
            mb.add_registration_code( convert_to_py_name+"();" )
        mb.add_registration_code( convert_from_py_name+"();" )
        
    
    def ParseImageClasses(self, mb):
        # IBorder
        cls = mb.class_('IBorder')
        cls.include()
        cls.mem_funs('ApplySchemeSettings').include()    
        cls.mem_funs('Paint').virtuality = 'pure virtual'
        cls.mem_funs('ApplySchemeSettings').virtuality = 'pure virtual'

        # IImage
        cls = mb.class_('IImage')
        cls.include()  
        cls.mem_funs( 'GetContentSize' ).add_transformation( FT.output('wide'), FT.output('tall') )
        cls.mem_funs( 'GetSize' ).add_transformation( FT.output('wide'), FT.output('tall') )
        cls.mem_funs('Paint').virtuality = 'pure virtual'
        
        # Image
        cls = mb.class_('Image')
        cls.include()
        #cls.mem_funs( matchers.access_type_matcher_t( 'protected' ) ).exclude()
        cls.no_init = True
        #cls.calldefs().virtuality = 'not virtual' 
        cls.calldefs('Image').exclude()
        cls.mem_funs( 'Paint' ).exclude()
        cls.add_wrapper_code( 'virtual void Paint() {}' )    # Stub for wrapper class. Otherwise it will complain.
        cls.mem_funs( 'GetSize' ).add_transformation( FT.output('wide'), FT.output('tall') )
        cls.mem_funs( 'GetContentSize' ).add_transformation( FT.output('wide'), FT.output('tall') )
        
        # FIXME: Py++ is giving problems on some functions
        cls.mem_funs('SetPos').virtuality = 'not virtual'
        #cls.mem_funs('GetPos').virtuality = 'not virtual'
        #cls.mem_funs('GetSize').virtuality = 'not virtual'
        #cls.mem_funs('GetContentSize').virtuality = 'not virtual'
        cls.mem_funs('SetColor').virtuality = 'not virtual'
        cls.mem_funs('SetBkColor').virtuality = 'not virtual'
        cls.mem_funs('GetColor').virtuality = 'not virtual'
        
        cls.mem_funs('SetSize').virtuality = 'not virtual' 
        cls.mem_funs('DrawSetColor').virtuality = 'not virtual' 
        cls.mem_funs('DrawSetColor').virtuality = 'not virtual' 
        cls.mem_funs('DrawFilledRect').virtuality = 'not virtual' 
        cls.mem_funs('DrawOutlinedRect').virtuality = 'not virtual' 
        cls.mem_funs('DrawLine').virtuality = 'not virtual' 
        cls.mem_funs('DrawPolyLine').virtuality = 'not virtual' 
        cls.mem_funs('DrawSetTextFont').virtuality = 'not virtual' 
        cls.mem_funs('DrawSetTextColor').virtuality = 'not virtual' 
        cls.mem_funs('DrawSetTextPos').virtuality = 'not virtual' 
        cls.mem_funs('DrawPrintText').virtuality = 'not virtual' 
        cls.mem_funs('DrawPrintText').virtuality = 'not virtual' 
        cls.mem_funs('DrawPrintChar').virtuality = 'not virtual' 
        cls.mem_funs('DrawPrintChar').virtuality = 'not virtual' 
        cls.mem_funs('DrawSetTexture').virtuality = 'not virtual' 
        cls.mem_funs('DrawTexturedRect').virtuality = 'not virtual' 
        
        # TextImage
        cls = mb.class_('TextImage')
        cls.include()
        cls.calldefs().virtuality = 'not virtual' 
        cls.mem_funs( matchers.access_type_matcher_t( 'protected' ) ).exclude()
        #cls.calldefs('SetText', calldef_withtypes([pointer_t(const_t(declarated_t(wchar_t())))])).exclude()
        cls.mem_funs( 'GetContentSize' ).add_transformation( FT.output('wide'), FT.output('tall') )
        cls.mem_funs( 'GetDrawWidth' ).add_transformation( FT.output('width') )
        cls.mem_funs( 'SizeText' ).exclude()     # DECLARATION ONLY
        
        cls.mem_funs('GetText').exclude()
        cls.add_wrapper_code(
            'boost::python::object GetText() {\r\n' + \
            '    char buf[1025];\r\n' + \
            '    TextImage::GetText(buf, 1025);\r\n' + \
            '    return boost::python::object(buf);\r\n' + \
            '}'
        )
        cls.add_registration_code(
            'def( \r\n' + \
            '    "GetText"\r\n' + \
            '    , (boost::python::object ( TextImage_wrapper::* )())( &TextImage_wrapper::GetText ) )'
        )
        
        # BitmapImage
        cls = mb.class_('BitmapImage')
        cls.include()
        cls.calldefs().virtuality = 'not virtual' 
        if self.settings.branch == 'source2013':
            cls.mem_fun('SetBitmap').exclude()
        #cls.mem_funs( matchers.access_type_matcher_t( 'protected' ) ).exclude()
        
        cls.calldefs('GetColor', calldef_withtypes([reference_t(declarated_t(int_t()))])).add_transformation(FT.output('r'), FT.output('g'), FT.output('b'), FT.output('a'))
        cls.mem_funs( 'GetSize' ).add_transformation( FT.output('wide'), FT.output('tall') )
        
        if self.settings.branch == 'swarm':
            # CAvatarImage
            cls = mb.class_('CAvatarImage')
            cls.include()
            cls.calldefs().virtuality = 'not virtual' 
            cls.mem_funs( matchers.access_type_matcher_t( 'protected' ) ).exclude()
            cls.rename('AvatarImage')
            cls.mem_funs( 'GetSize' ).add_transformation( FT.output('wide'), FT.output('tall') )
            cls.mem_funs( 'GetContentSize' ).add_transformation( FT.output('wide'), FT.output('tall') )
            cls.mem_funs( 'InitFromRGBA' ).exclude()
        
            mb.enum('EAvatarSize').include()
    
    def ParsePanelHandles(self, mb):
        # Base handle for Panels
        mb.class_('PyBaseVGUIHandle').include()
        mb.class_('PyBaseVGUIHandle').mem_funs().exclude()
        
    def GetWrapper(self, cls, methodname):
        wrapper = None
        try:
            wrapper = calldef.mem_fun_v_wrapper_t( cls.mem_fun(methodname) )
        except: #matcher.declaration_not_found_t:
            for base in cls.recursive_bases:
                wrapper = self.GetWrapper(base.related_class, methodname)
                if wrapper:
                    break
        return wrapper

    def ParsePanels(self, mb):
        # Panels
        cls = mb.class_('DeadPanel')
        cls.include()
        cls.mem_fun('NonZero').rename('__nonzero__')

        # For each panel sub class we take some default actions
        for cls_name in self.panel_cls_list:
            cls = mb.class_(cls_name)

            # Include everything by default
            cls.include()
            cls.no_init = False
            
            # Be selective about we need to override
            cls.mem_funs().virtuality = 'not virtual' 

            cls.add_fake_base('PyPanel')
            
            wrapperpaint = self.GetWrapper( cls, 'Paint' )
            wrapperpaintbackground = self.GetWrapper( cls, 'PaintBackground' )
            wrapperinvalidatelayout = self.GetWrapper( cls, 'InvalidateLayout' )
            cls.add_wrapper_code(r'''
    virtual void Paint(  ) {
        if( !IsSBufferEnabled() || ShouldRecordSBuffer( m_PaintCallBuffer ) )
        {
            PY_OVERRIDE_CHECK( %(cls_name)s, Paint )
            PY_OVERRIDE_LOG( %(modulename)s, %(cls_name)s, Paint )
            bp::override func_Paint = this->get_override( "Paint" );
            if( func_Paint.ptr() != Py_None )
                try {
                    func_Paint(  );
                } catch(bp::error_already_set &) {
                    PyErr_Print();
                    this->%(cls_name)s::Paint(  );
                }
            else
                this->%(cls_name)s::Paint(  );
                
            FinishRecordSBuffer( m_PaintCallBuffer );
 
        }
        else if( IsSBufferEnabled() )
        {
            DrawFromSBuffer( m_PaintCallBuffer );
        }
    }
    
    void default_Paint(  ) {
        %(cls_name)s::Paint( );
    }

    virtual void PaintBackground(  ) {
        if( !IsSBufferEnabled() || ShouldRecordSBuffer( m_PaintBackgroundCallBuffer ) )
        {
            PY_OVERRIDE_CHECK( %(cls_name)s, PaintBackground )
            PY_OVERRIDE_LOG( %(modulename)s, %(cls_name)s, PaintBackground )
            bp::override func_PaintBackground = this->get_override( "PaintBackground" );
            if( func_PaintBackground.ptr() != Py_None )
                try {
                    func_PaintBackground(  );
                } catch(bp::error_already_set &) {
                    PyErr_Print();
                    this->%(pb_cls_name)s::PaintBackground(  );
                }
            else
                this->%(pb_cls_name)s::PaintBackground(  );
                
            FinishRecordSBuffer( m_PaintBackgroundCallBuffer );
        }
        else if( IsSBufferEnabled() )
        {
            DrawFromSBuffer( m_PaintBackgroundCallBuffer );
        }
    }
    
    void default_PaintBackground(  ) {
        %(pb_cls_name)s::PaintBackground( );
    }
    
    virtual void InvalidateLayout( bool layoutNow=false, bool reloadScheme=false ) {
        FlushSBuffer();
        
        PY_OVERRIDE_CHECK( %(cls_name)s, InvalidateLayout )
        PY_OVERRIDE_LOG( %(modulename)s, %(cls_name)s, InvalidateLayout )
        bp::override func_InvalidateLayout = this->get_override( "InvalidateLayout" );
        if( func_InvalidateLayout.ptr() != Py_None )
            try {
                func_InvalidateLayout( layoutNow, reloadScheme );
            } catch(bp::error_already_set &) {
                PyErr_Print();
                this->%(il_cls_name)s::InvalidateLayout( layoutNow, reloadScheme );
            }
        else
            this->%(il_cls_name)s::InvalidateLayout( layoutNow, reloadScheme );
    }
    
    void default_InvalidateLayout( bool layoutNow=false, bool reloadScheme=false ) {
        FlushSBuffer();
        %(il_cls_name)s::InvalidateLayout( layoutNow, reloadScheme );
    }
    ''' %   {   'cls_name' : wrapperpaint.wrapped_class_identifier(),
                'pb_cls_name' : wrapperpaintbackground.wrapped_class_identifier(),
                'il_cls_name' : wrapperinvalidatelayout.wrapped_class_identifier(),
                'modulename' : self.module_name,
            })
            
            cls.add_registration_code(r'''
{ //::vgui::%(cls_name)s::Paint

        typedef void ( ::vgui::Panel::*Paint_function_type )(  ) ;
        typedef void ( %(cls_name)s_wrapper::*default_Paint_function_type )(  ) ;
        
        %(cls_name)s_exposer.def( 
            "Paint"
            , Paint_function_type(&::vgui::Panel::Paint)
            , default_Paint_function_type(&%(cls_name)s_wrapper::default_Paint) );

    }
    { //::vgui::%(cls_name)s::PaintBackground

        typedef void ( ::vgui::Panel::*PaintBackground_function_type )(  ) ;
        typedef void ( %(cls_name)s_wrapper::*default_PaintBackground_function_type )(  ) ;
        
        %(cls_name)s_exposer.def( 
            "PaintBackground"
            , PaintBackground_function_type(&::vgui::Panel::PaintBackground)
            , default_PaintBackground_function_type(&%(cls_name)s_wrapper::default_PaintBackground) );
    
    }
    { //::vgui::%(cls_name)s::InvalidateLayout
    
        typedef void ( ::vgui::Panel::*InvalidateLayout_function_type )( bool,bool ) ;
        typedef void ( %(cls_name)s_wrapper::*default_InvalidateLayout_function_type )( bool,bool ) ;
        
        %(cls_name)s_exposer.def( 
            "InvalidateLayout"
            , InvalidateLayout_function_type(&::vgui::Panel::InvalidateLayout)
            , default_InvalidateLayout_function_type(&%(cls_name)s_wrapper::default_InvalidateLayout)
            , ( bp::arg("layoutNow")=(bool)(false), bp::arg("reloadScheme")=(bool)(false) ) );
    
    }
    ''' % {'cls_name' : cls_name}, False)
            
            # By default exclude any subclass. These classes are likely controlled intern by the panel
            if cls.classes(allow_empty=True):
                cls.classes().exclude()
                
            self.AddVGUIConverter(mb, cls_name, self.novguilib, containsabstract=False)
            
            # # Add custom wrappers for functions who take keyvalues as input
            if self.novguilib:
                # No access to source code, so need to add message stuff for python here.
                cls.add_wrapper_code('virtual void OnMessage(const KeyValues *params, VPANEL fromPanel) {\r\n' +
                                     '    if( Panel_DispatchMessage( m_PyMessageMap, params, fromPanel ) )\r\n' +
                                     '        return;\r\n' +
                                     '    Panel::OnMessage(params, fromPanel);\r\n' +
                                     '}\r\n' + \
                                     '\r\n' + \
                                     'void RegMessageMethod( const char *message, boost::python::object method, int numParams=0, \r\n' + \
                                     '       const char *nameFirstParam="", int typeFirstParam=DATATYPE_VOID, \r\n' + \
                                     '       const char *nameSecondParam="", int typeSecondParam=DATATYPE_VOID ) { \r\n' + \
                                     '       py_message_entry_t entry;\r\n' + \
                                     '       entry.method = method;\r\n' + \
                                     '       entry.numParams = numParams;\r\n' + \
                                     '       entry.firstParamName = nameFirstParam;\r\n' + \
                                     '       entry.firstParamSymbol = KeyValuesSystem()->GetSymbolForString(nameFirstParam);\r\n' + \
                                     '       entry.firstParamType = typeFirstParam;\r\n' + \
                                     '       entry.secondParamName = nameSecondParam;\r\n' + \
                                     '       entry.secondParamSymbol = KeyValuesSystem()->GetSymbolForString(nameSecondParam);\r\n' + \
                                     '       entry.secondParamType = typeSecondParam;\r\n' + \
                                     '\r\n' + \
                                     '       GetPyMessageMap().Insert(message, entry);\r\n' + \
                                     '}\r\n' + \
                                     'virtual Panel *GetPanel() { return this; }\r\n'
                                     )
                cls.add_registration_code('def( "RegMessageMethod", &'+cls_name+'_wrapper::RegMessageMethod\r\n' + \
                                               ', ( bp::arg("message"), bp::arg("method"), bp::arg("numParams")=(int)(0), bp::arg("nameFirstParam")="", bp::arg("typeFirstParam")=int(::vgui::DATATYPE_VOID), bp::arg("nameSecondParam")="", bp::arg("typeSecondParam")=int(::vgui::DATATYPE_VOID) ))' )
                                               
                # Add stubs
                cls.add_wrapper_code('virtual void EnableSBuffer( bool bUseBuffer ) { PyPanel::EnableSBuffer( bUseBuffer ); }')
                cls.add_registration_code('def( "EnableSBuffer", &%(cls_name)s_wrapper::EnableSBuffer, bp::arg("bUseBuffer") )' % {'cls_name' : cls_name})
                cls.add_wrapper_code('virtual bool IsSBufferEnabled() { return PyPanel::IsSBufferEnabled(); }')
                cls.add_registration_code('def( "IsSBufferEnabled", &%(cls_name)s_wrapper::IsSBufferEnabled )' % {'cls_name' : cls_name})
                cls.add_wrapper_code('virtual void FlushSBuffer() { PyPanel::FlushSBuffer(); }')
                cls.add_registration_code('def( "FlushSBuffer", &%(cls_name)s_wrapper::FlushSBuffer )' % {'cls_name' : cls_name})
                cls.add_wrapper_code('virtual void SetFlushedByParent( bool bEnabled ) { PyPanel::SetFlushedByParent( bEnabled ); }')
                cls.add_registration_code('def( "SetFlushedByParent", &%(cls_name)s_wrapper::SetFlushedByParent, bp::arg("bEnabled") )' % {'cls_name' : cls_name})
    
        # Tweak Panels
        # Used by converters + special method added in the wrapper
        # Don't include here
        if not self.novguilib:
            mb.mem_funs('GetPySelf').exclude()
            mb.mem_funs('PyDestroyPanel').exclude()
            
        # Exclude message stuff. Maybe look into wrapping this in a nice way
        mb.mem_funs( 'AddToMap' ).exclude()
        mb.mem_funs( 'ChainToMap' ).exclude()
        mb.mem_funs( 'GetMessageMap' ).exclude()
        mb.mem_funs( 'AddToAnimationMap' ).exclude()
        mb.mem_funs( 'ChainToAnimationMap' ).exclude()
        mb.mem_funs( 'GetAnimMap' ).exclude()
        mb.mem_funs( 'KB_AddToMap' ).exclude()
        mb.mem_funs( 'KB_ChainToMap' ).exclude()
        mb.mem_funs( 'KB_AddBoundKey' ).exclude()
        mb.mem_funs( 'GetKBMap' ).exclude()
        mb.mem_funs( lambda decl: 'GetVar_' in decl.name ).exclude()
        
        mb.classes( lambda decl: 'PanelMessageFunc_' in decl.name ).exclude()
        mb.classes( lambda decl: '_Register' in decl.name ).exclude()
        mb.classes( lambda decl: 'PanelAnimationVar_' in decl.name ).exclude()
        mb.vars( lambda decl: '_register' in decl.name ).exclude()
        mb.vars( lambda decl: 'm_Register' in decl.name ).exclude()
        
        # Don't need the following:
        menu = mb.class_('Menu')
        keybindindcontexthandle = mb.enum('KeyBindingContextHandle_t')
        excludetypes = [
            pointer_t(const_t(declarated_t(menu))),
            pointer_t(declarated_t(menu)),
            reference_t(declarated_t(menu)),
            pointer_t(declarated_t(mb.class_('IPanelAnimationPropertyConverter'))),
            declarated_t(keybindindcontexthandle),
        ]
        mb.calldefs(calldef_withtypes(excludetypes), allow_empty=True).exclude()
        
    def ParsePanel(self, mb):
        # List of functions that should be overridable
        mb.mem_funs('SetVisible').virtuality = 'virtual'
        mb.mem_funs('SetParent').virtuality = 'virtual'
        mb.mem_funs('SetEnabled').virtuality = 'virtual'
        
        mb.mem_funs('SetBgColor').virtuality = 'virtual'
        mb.mem_funs('SetFgColor').virtuality = 'virtual'
        mb.mem_funs('SetCursor').virtuality = 'virtual'
        
        mb.mem_funs('InvalidateLayout').virtuality = 'virtual'
        
        mb.mem_funs('SetBorder').virtuality = 'virtual'
        mb.mem_funs('SetPaintBorderEnabled').virtuality = 'virtual'
        mb.mem_funs('SetPaintBackgroundEnabled').virtuality = 'virtual'
        mb.mem_funs('SetPaintEnabled').virtuality = 'virtual'
        mb.mem_funs('SetPaintBackgroundType').virtuality = 'virtual'
        mb.mem_funs('SetScheme').virtuality = 'virtual'
        
        mb.mem_funs('ApplySchemeSettings').virtuality = 'virtual'
        mb.mem_funs('OnCommand').virtuality = 'virtual'
        mb.mem_funs('OnMouseCaptureLost').virtuality = 'virtual'
        mb.mem_funs('OnSetFocus').virtuality = 'virtual'
        mb.mem_funs('OnKillFocus').virtuality = 'virtual'
        mb.mem_funs('OnDelete').virtuality = 'virtual'
        mb.mem_funs('OnThink').virtuality = 'virtual'
        mb.mem_funs('OnChildAdded').virtuality = 'virtual'
        mb.mem_funs('OnSizeChanged').virtuality = 'virtual'
        mb.mem_funs('OnTick').virtuality = 'virtual'
        
        mb.mem_funs('OnCursorMoved').virtuality = 'virtual'
        mb.mem_funs('OnCursorEntered').virtuality = 'virtual'
        mb.mem_funs('OnCursorExited').virtuality = 'virtual'
        mb.mem_funs('OnMousePressed').virtuality = 'virtual'
        mb.mem_funs('OnMouseDoublePressed').virtuality = 'virtual'
        mb.mem_funs('OnMouseReleased').virtuality = 'virtual'
        mb.mem_funs('OnMouseWheeled').virtuality = 'virtual'
        mb.mem_funs('OnMouseTriplePressed').virtuality = 'virtual'
        
        mb.mem_funs('OnKeyCodePressed').virtuality = 'virtual'
        mb.mem_funs('OnKeyCodeTyped').virtuality = 'virtual'
        mb.mem_funs('OnKeyCodeReleased').virtuality = 'virtual'
        mb.mem_funs('OnKeyFocusTicked').virtuality = 'virtual'
        mb.mem_funs('OnMouseFocusTicked').virtuality = 'virtual'
        
        mb.mem_funs('PaintBackground').virtuality = 'virtual'
        mb.mem_funs('Paint').virtuality = 'virtual'
        #mb.mem_funs('PaintBorder').virtuality = 'virtual' # TODO: Don't believe we are ever interested in painting borders in python
        mb.mem_funs('PaintBuildOverlay').virtuality = 'virtual'
        mb.mem_funs('PostChildPaint').virtuality = 'virtual'
        mb.mem_funs('PerformLayout').virtuality = 'virtual'
        
        mb.mem_funs('SetMouseInputEnabled').virtuality = 'virtual'
        mb.mem_funs('SetKeyBoardInputEnabled').virtuality = 'virtual'
        
        mb.mem_funs('SetDragEnabled').virtuality = 'virtual'
        
        mb.mem_funs('OnRequestFocus').virtuality = 'virtual'
        mb.mem_funs('OnScreenSizeChanged').virtuality = 'virtual'

        # Transformations
        mb.mem_funs( 'GetPos' ).add_transformation( FT.output('x'), FT.output('y') )
        mb.class_('Panel').mem_funs( 'GetSize' ).add_transformation( FT.output('wide'), FT.output('tall') )
        mb.class_('Panel').mem_funs( 'GetBounds' ).add_transformation( FT.output('x'), FT.output('y'), FT.output('wide'), FT.output('tall') )
        mb.mem_funs( 'GetMinimumSize' ).add_transformation( FT.output('wide'), FT.output('tall') )
        mb.mem_funs( 'LocalToScreen' ).add_transformation( FT.inout('x'), FT.inout('y') )
        mb.mem_funs( 'ScreenToLocal' ).add_transformation( FT.inout('x'), FT.inout('y') )
        mb.mem_funs( 'ParentLocalToScreen' ).add_transformation( FT.inout('x'), FT.inout('y') )
        mb.mem_funs( 'GetInset' ).add_transformation( FT.output('left'), FT.output('top'), FT.output('right'), FT.output('bottom') )
        mb.mem_funs( 'GetPaintSize' ).add_transformation( FT.output('wide'), FT.output('tall') )
        mb.mem_funs( 'GetClipRect' ).add_transformation( FT.output('x0'), FT.output('y0'), FT.output('x1'), FT.output('y1') )
        mb.mem_funs( 'GetPinOffset' ).add_transformation( FT.output('dx'), FT.output('dy') )
        mb.mem_funs( 'GetResizeOffset' ).add_transformation( FT.output('dx'), FT.output('dy') )
        mb.mem_funs( 'GetCornerTextureSize' ).add_transformation( FT.output('w'), FT.output('h') )    
        
        # Exclude list
        mb.mem_funs('QueryInterface').exclude()
        
        # Custom implemented
        mb.mem_funs('PaintBackground').exclude()
        mb.mem_funs('Paint').exclude()
        mb.mem_funs('InvalidateLayout').exclude()
        
        # We don't care about build mode, since we can easily reload modules in python
        # We also don't want the user to be able to call methods like Delete.
        mb.mem_funs('IsBuildModeEditable').exclude()
        mb.mem_funs('SetBuildModeEditable').exclude()
        mb.mem_funs('IsBuildModeDeletable').exclude()
        mb.mem_funs('SetBuildModeDeletable').exclude()
        mb.mem_funs('IsBuildModeActive').exclude()
        #mb.mem_funs('SetAutoDelete').exclude()
        #mb.mem_funs('IsAutoDeleteSet').exclude()
        mb.mem_funs('OnDelete').exclude()
        mb.mem_funs('MarkForDeletion').exclude()
        mb.mem_funs('SetBuildGroup').exclude()
        mb.mem_funs('IsBuildGroupEnabled').exclude()
        mb.mem_funs('CreateControlByName').exclude()
        
        mb.mem_funs('LoadKeyBindings').exclude()
        mb.mem_funs('SaveKeyBindingsToBuffer').exclude()
        mb.mem_funs('LookupBoundKeys').exclude()
        mb.mem_funs('OnKeyTyped').exclude()
        mb.mem_funs('HasHotkey').exclude()

        mb.mem_funs('GetDragData').exclude()
        if self.settings.branch == 'swarm':
            mb.mem_funs('GetDragFailCursor').exclude()
        mb.mem_funs('GetDropCursor').exclude()
        mb.mem_funs('GetDropTarget').exclude()
        mb.mem_funs('IsDroppable').exclude()
        mb.mem_funs('OnPanelDropped').exclude()
        mb.mem_funs('OnPanelEnteredDroppablePanel').exclude()
        mb.mem_funs('OnPanelExitedDroppablePanel').exclude()
        mb.mem_funs('OnDragFailed').exclude()
        mb.mem_funs('OnDropContextHoverHide').exclude()
        mb.mem_funs('OnDropContextHoverShow').exclude()
        mb.mem_funs('OnDroppablePanelPaint').exclude()
        mb.mem_funs('OnGetAdditionalDragPanels').exclude()
        mb.mem_funs('OnDragFailed').exclude()
        
        mb.vars('m_PanelMap').exclude()
        mb.vars('m_MessageMap').exclude()
        mb.mem_funs('GetPanelMap').exclude()
        if self.settings.branch == 'source2013':
            mb.mem_funs('GetChildren').exclude()
       
        # Must use return_by_value. Then the converter will be used to wrap the vgui element in a safe handle
        mb.mem_funs( 'GetChild' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        mb.mem_funs( 'GetBorder' ).call_policies = call_policies.return_value_policy( call_policies.reference_existing_object )
        if self.settings.branch == 'source2007':
            mb.mem_funs('GetBorderAtIndex').call_policies = call_policies.return_value_policy( call_policies.reference_existing_object )
        mb.mem_funs( 'GetParent' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        mb.mem_funs( 'FindSiblingByName' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        mb.mem_funs( 'FindChildByName' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        mb.mem_funs( 'HasHotkey' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        mb.mem_funs( 'GetPanelWithKeyBindings' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        mb.mem_funs( 'LookupBinding' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        mb.mem_funs( 'LookupBindingByKeyCode' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        mb.mem_funs( 'LookupDefaultKey' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        mb.mem_funs( 'LookupMapForBinding' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        mb.mem_funs( 'GetTooltip' ).call_policies = call_policies.return_internal_reference()  
        mb.mem_funs( 'GetDragDropInfo' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        mb.mem_funs( 'GetDragPanel' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        mb.mem_funs( 'GetPanel' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        mb.mem_funs( 'FindPanelAnimationEntry' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        mb.mem_funs( 'FindDropTargetPanel' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
        
        if self.settings.branch == 'swarm':
            mb.mem_funs( 'GetNavDown' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
            mb.mem_funs( 'GetNavDownPanel' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
            mb.mem_funs( 'GetNavLeft' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
            mb.mem_funs( 'GetNavLeftPanel' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
            mb.mem_funs( 'GetNavRight' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
            mb.mem_funs( 'GetNavRightPanel' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
            mb.mem_funs( 'GetNavUp' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
            mb.mem_funs( 'GetNavUpPanel' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
            mb.mem_funs( 'NavigateDown' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
            mb.mem_funs( 'NavigateLeft' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
            mb.mem_funs( 'NavigateRight' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
            mb.mem_funs( 'NavigateTo' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
            mb.mem_funs( 'NavigateUp' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
            mb.mem_funs( 'SetNavDown' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
            mb.mem_funs( 'SetNavLeft' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
            mb.mem_funs( 'SetNavRight' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
            mb.mem_funs( 'SetNavUp' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value) 

            # Exclude
            mb.mem_funs('OnUnserialized').exclude()
            mb.mem_funs('GetSizer').exclude()
            mb.mem_funs('GetUnpackStructure').exclude()
            
        if self.settings.branch == 'swarm':
            # Tooltip class
            cls = mb.class_('Tooltip')
            cls.include()
        
    def ParseEditablePanel(self, mb):
        focusnavgroup = mb.class_('FocusNavGroup')
        buildgroup = mb.class_('BuildGroup')
        excludetypes = [
            pointer_t(const_t(declarated_t(focusnavgroup))),
            pointer_t(declarated_t(focusnavgroup)),
            reference_t(declarated_t(focusnavgroup)),
            pointer_t(const_t(declarated_t(buildgroup))),
            pointer_t(declarated_t(buildgroup)),
            reference_t(declarated_t(buildgroup)),
        ]
        mb.calldefs(calldef_withtypes(excludetypes), allow_empty=True).exclude()
        
        mb.mem_funs( 'GetDialogVariables' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        
    def ParseFrame(self, mb):
        # List of overridables
        mb.mem_funs('SetTitle').virtuality = 'virtual'
        mb.mem_funs('Activate').virtuality = 'virtual'
        mb.mem_funs('ActivateMinimized').virtuality = 'virtual'
        mb.mem_funs('Close').virtuality = 'virtual'
        mb.mem_funs('CloseModal').virtuality = 'virtual'
        mb.mem_funs('SetMoveable').virtuality = 'virtual'
        mb.mem_funs('SetSizeable').virtuality = 'virtual'
        mb.mem_funs('SetMenuButtonVisible').virtuality = 'virtual'
        mb.mem_funs('SetMinimizeButtonVisible').virtuality = 'virtual'
        mb.mem_funs('SetMaximizeButtonVisible').virtuality = 'virtual'
        mb.mem_funs('SetMinimizeToSysTrayButtonVisible').virtuality = 'virtual'
        mb.mem_funs('SetCloseButtonVisible').virtuality = 'virtual'
        mb.mem_funs('FlashWindow').virtuality = 'virtual'
        mb.mem_funs('FlashWindowStop').virtuality = 'virtual'
        mb.mem_funs('SetTitleBarVisible').virtuality = 'virtual'
        mb.mem_funs('SetClipToParent').virtuality = 'virtual'
        mb.mem_funs('SetSmallCaption').virtuality = 'virtual'
        mb.mem_funs('DoModal').virtuality = 'virtual'
        mb.mem_funs('OnClose').virtuality = 'virtual'
        mb.mem_funs('OnFinishedClose').virtuality = 'virtual'
        mb.mem_funs('OnMinimize').virtuality = 'virtual'
        mb.mem_funs('OnMinimizeToSysTray').virtuality = 'virtual'
        mb.mem_funs('OnCloseFrameButtonPressed').virtuality = 'virtual'
        mb.mem_funs('GetDefaultScreenPosition').virtuality = 'virtual'
        mb.mem_funs('OnCloseFrameButtonPressed').virtuality = 'virtual'
        mb.mem_funs('OnCloseFrameButtonPressed').virtuality = 'virtual'
        mb.mem_funs('OnCloseFrameButtonPressed').virtuality = 'virtual'
        mb.mem_funs('OnCloseFrameButtonPressed').virtuality = 'virtual'
        mb.mem_funs('OnCloseFrameButtonPressed').virtuality = 'virtual'
        mb.mem_funs('OnCloseFrameButtonPressed').virtuality = 'virtual'
        
        mb.mem_funs('SetDeleteSelfOnClose').exclude()
        mb.mem_funs('GetSysMenu').exclude()
        
        mb.mem_funs( 'GetDefaultScreenPosition' ).add_transformation( FT.output('x'), FT.output('y'), FT.output('wide'), FT.output('tall') ) 
        mb.mem_funs( 'GetClientArea' ).add_transformation( FT.output('x'), FT.output('y'), FT.output('wide'), FT.output('tall') ) 

    def ScrollBar(self, mb):
        mb.mem_funs( 'GetButton' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        mb.mem_funs( 'GetSlider' ).call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        
        mb.mem_funs( 'GetRange' ).add_transformation( FT.output('min'), FT.output('max') ) 

        # ScrollBarSlider
        mb.mem_funs( 'GetNobPos' ).add_transformation( FT.output('min'), FT.output('max') ) 
    
    def ParseAnimationController(self, mb):
        # Make empty, don't care
        #self.IncludeEmptyClass(mb, 'AnimationController')
        pass
        
    
    def ParseTextEntry(self, mb):
        # List of overridables
        mb.mem_funs('SetText').virtuality = 'virtual'
        mb.mem_funs('MoveCursor').virtuality = 'virtual'
        mb.mem_funs('SetDisabledBgColor').virtuality = 'virtual'
        mb.mem_funs('SetMultiline').virtuality = 'virtual'
        mb.mem_funs('SetVerticalScrollbar').virtuality = 'virtual'

        #
        mb.mem_funs('GetEditMenu').exclude()        # Exclude for now, add back later when we found out call policies.
        
        mb.mem_funs( 'GetSelectedRange' ).add_transformation( FT.output('cx0'), FT.output('cx1') )   
        mb.mem_funs( 'CursorToPixelSpace' ).add_transformation( FT.inout('cx'), FT.inout('cy') ) 
        mb.mem_funs( 'AddAnotherLine' ).add_transformation( FT.output('cx'), FT.output('cy') ) 
        mb.mem_funs( 'GetStartDrawIndex' ).add_transformation( FT.output('lineBreakIndexIndex') ) 
        
        # Wrap GetText manual
        mb.class_('TextEntry').mem_funs('GetText').exclude()
        mb.class_('TextEntry').add_wrapper_code(
            'boost::python::object GetText() {\r\n' + \
            '    const char *buf = (const char *)malloc( (GetTextLength()+1)*sizeof(char) );\r\n' + \
            '    TextEntry::GetText((char *)buf, GetTextLength()+1);\r\n' + \
            '    boost::python::object rv(buf);\r\n' + \
            '    delete buf;\r\n' + \
            '    return rv;\r\n' + \
            '}'
        )
        
        mb.class_('TextEntry').add_registration_code(
            'def( \r\n' + \
            '    "GetText"\r\n' + \
            '    , (boost::python::object ( TextEntry_wrapper::* )())( &TextEntry_wrapper::GetText ) )'
        )

        # RichText
        if self.settings.branch == 'swarm':
            mb.mem_funs('GetScrollBar').exclude()
            
    def ParseMisc(self, mb):
        cls = mb.class_('VideoGeneralPanel')
        cls.mem_funs('OnVideoOver').virtuality = 'virtual'
        
        cls.mem_funs('SetVideoFlags').exclude()
        cls.mem_funs('GetVideoFlags').exclude()
        
        mb.class_('VideoGeneralPanel').add_property( 'videoflags'
                         , cls.member_function( 'GetVideoFlags' )
                         , cls.member_function( 'SetVideoFlags' ) )
                         
        mb.add_registration_code( "bp::scope().attr( \"BIK_LOOP\" ) = BIK_LOOP;" )
        mb.add_registration_code( "bp::scope().attr( \"BIK_PRELOAD\" ) = BIK_PRELOAD;" )
        mb.add_registration_code( "bp::scope().attr( \"BIK_NO_AUDIO\" ) = BIK_NO_AUDIO;" )
        
    def TestBasePanel(self, cls):
        basepanelcls = self.basepanelcls
        recursive_bases = cls.recursive_bases
        for testcls in recursive_bases:
            if basepanelcls == testcls.related_class:
                return True
        return cls == basepanelcls
        
    def Parse(self, mb):
        self.novguilib = (self.settings.branch == 'swarm')
        
        # Exclude everything by default
        mb.decls().exclude()  

        self.ParsePanelHandles(mb)
        self.ParsePanels(mb)
        
        self.ParsePanel(mb)
        self.ParseEditablePanel(mb)
        self.ParseFrame(mb)
        self.ScrollBar(mb)
        self.ParseAnimationController(mb)
        self.ParseTextEntry(mb)
        self.ParseMisc(mb)
        
        # Should already be included, but is for some reason not...
        mb.mem_funs('SetControlEnabled').include()
        
        # Anything that returns a Panel should be returned by Value to call the right converter
        testinherit = MatcherTestInheritClass(mb.class_('Panel'))
        decls = mb.calldefs(matchers.custom_matcher_t(testinherit))
        decls.call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        
        # All CBaseEntity related classes should have a custom call trait
        self.basepanelcls = mb.class_('Panel')
        def panel_call_trait(type_):
            return 'boost::python::object(*%(arg)s)'
        panelclasses = mb.classes(self.TestBasePanel)
        for panelclass in panelclasses:
            panelclass.custom_call_trait = panel_call_trait
        
        # Remove any protected function
        #mb.calldefs( matchers.access_type_matcher_t( 'protected' ) ).exclude()
        
        self.ParseImageClasses(mb)
        
        self.ApplyCommonRules(mb)
        