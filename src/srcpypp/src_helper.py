# Helper module. Common methods.
from pygccxml import parser
from pyplusplus.code_creators import calldef
from pyplusplus.code_creators import code_creator
from pygccxml import declarations
from pyplusplus import messages
import os

import settings

# ---------------------------------------------------------
# Check if the function has a certain argument
def HasArgType( decl, arg_type ):
    for argtype in decl.arguments:
        if argtype.type.decl_string.find(arg_type) != -1:
            return True
            
    return False
	
def HasArgTypes( decl, arg_types ):
    for arg_type in arg_types:
        if HasArgType(decl, arg_type) == False:
            return False
    return True

# Including stuff
def IncludeEmptyClass( mb, cls_name ):
    c = mb.class_(cls_name)
    c.include()
    if c.classes(allow_empty=True):
        c.classes(allow_empty=True).exclude()
    if c.mem_funs(allow_empty=True):
        c.mem_funs(allow_empty=True).exclude()
    if c.vars(allow_empty=True):
        c.vars(allow_empty=True).exclude()
    if c.enums(allow_empty=True):
        c.enums(allow_empty=True).exclude()       
    
def IncludeEmptyClasses( mb, class_list ):
    for cls in class_list:
        IncludeEmptyClass( mb, cls )
    
# ---------------------------------------------------------
# Replacing things ( can be removed I think )    
def RemoveLine( content, line ):
    s = content.partition(line)
    return s[0] + s[1]
    
def ReplaceLine( content, line, template ):
    s = content.partition(line)
    return s[0] + TemplateToText(template) + s[2]   
    
def FileToText( path ):
    f = open(path)
    content = f.read()
    f.close()
    return content
    
def TextToParserConfig( content ):
    return parser.create_text_fc(content)
    
def TemplateToText( template ):
    text = ""
    for line in template:
        text = text + line + "\r\n"
    return text

def FileToTextWithReplacement( path, replace_line, template ):
    f = open(path)
    content = f.read()
    f.close()
    content = ReplaceLine(content, replace_line, template)
    return parser.create_text_fc(content)

# ---------------------------------------------------------
# Add wrapper code   
def GenerateArgTypes( arg_types ):
    rs = ""
    for argt in arg_types:
        if rs != "":
            rs += ", "
        rs += argt
    return rs
    
def GenerateArgNames( arg_names ):
    rs = ""
    for argt in arg_names:
        if rs != "":
            rs += ", "
        rs += argt
    return rs
    
def GenerateArgBoostNames( arg_names ):
    rs = ""
    for argt in arg_names:
        if rs != "":
            rs += ", "
        rs += "bp::arg(\""+argt+"\")"
    return rs
    
def GenerateArgs( arg_types, arg_names ):
    rs = ""
    for i in range(0, len(arg_types)):
        if rs != "":
            rs += ", "
        rs += arg_types[i] + " " + arg_names[i]
    return rs
    
# ---------------------------------------------------------
class hackcodecreator_t( code_creator.code_creator_t ):
    def __init__(self, fullname):
        self.full_name = fullname

# ...
def GetTopDeclaration( mb, cls_name, decl_name, args = None ):
    decl = None
    cls = mb.class_(cls_name)
    function = None
    if args != None:
        function = lambda decl: HasArgTypes(decl, args)

    while decl == None and cls != None: 
        rs = cls.mem_funs(decl_name, function, allow_empty=True)
        if len(rs) == 1:    # Found the top declaration
            decl = cls.mem_fun(decl_name, function)
        elif len(rs) == 0:  # Search one parent up
            if len(cls.recursive_bases) > 0:
                cls = cls.recursive_bases[0].related_class
            else:
                cls = None
        else:               # Must be more specific
            print('Multiple declarations found. Specify more specific arguments')
            return None
    return decl
    
# ...
def AddWrapRegs(mb, cls_name, declarations, args_mod_in):
    for decl in declarations:
        AddWrapReg( mb, cls_name, decl, args_mod_in )
        
def AddWrapReg(mb, cls_name, declaration, args_mod_in, nobasefn=False):
    if type(cls_name) != str:
        cls = cls_name
        cls_name = cls.name # or alias?
    else:
        cls = mb.class_(cls_name)
    
    # Create a wrapper class inst
    wrapper = calldef.mem_fun_v_wrapper_t( declaration )

    template = []
    
    # NOTE: Comparing directly gives problems
    template.append( '%(override)s func_%(alias)s = this->get_override( "%(alias)s" );' )
    template.append( 'if( func_%(alias)s.ptr() != Py_None )' )
    template.append( wrapper.indent('try {' ) )
    template.append( wrapper.indent(wrapper.indent('%(return_)sfunc_%(alias)s( %(args_mod_in)s );' ) ) )
    template.append( wrapper.indent('} catch(...) {') )
    template.append( wrapper.indent(wrapper.indent('PyErr_Print();')) )
    if not nobasefn:
        template.append( wrapper.indent(wrapper.indent('%(return_)sthis->%(wrapped_class)s::%(name)s( %(args)s );') ) )
    template.append( wrapper.indent( '}' ) )
    if not nobasefn:    
        template.append( 'else' )
        template.append( wrapper.indent('%(return_)sthis->%(wrapped_class)s::%(name)s( %(args)s );') )
    
    template = os.linesep.join( template )

    return_ = ''
    if not declarations.is_void( wrapper.declaration.return_type ):
        return_ = 'return '
        
    answer = [ wrapper.create_declaration(wrapper.declaration.partial_name) + '{' ]
    answer.append( wrapper.indent( 
        template % {
            'override' : wrapper.override_identifier()
            , 'name' : wrapper.declaration.partial_name
            , 'alias' : wrapper.declaration.alias
            , 'return_' : return_
            , 'args' : wrapper.function_call_args()
            , 'args_mod_in' : GenerateArgNames(args_mod_in)
            , 'wrapped_class' : wrapper.wrapped_class_identifier()
        }      
    ) )
    answer.append( '}' )
    cls.add_wrapper_code( 
        os.linesep.join( answer )
    )
    
    if not nobasefn:
        # Add default body
        cls.add_wrapper_code( wrapper.create_default_function() )
    
    # Add registration code
    if nobasefn:
        reg = calldef.mem_fun_pv_t(declaration, wrapper)
    else:
        reg = calldef.mem_fun_v_t(declaration, wrapper)
    # WTF, why does it needs to be a code_creator instance. It only uses it for full_name
    cc = hackcodecreator_t(cls_name+'_wrapper')      
    wrapper.parent = cc
    cls.add_registration_code( reg._create_impl(), True )
    
def CreateEntityArg(entity_arg):
    return '%s ? %s->GetPyHandle() : bp::object()' % (entity_arg, entity_arg)
    
def CreateIHandleEntityArg(entity_arg):
    return 'ConvertIHandleEntity( %s )' % (entity_arg)
    
# ---------------------------------------------------------
# Add a network variable as property to a class
def AddNetworkVarProperty(mb, expose_name, var_name, type, cls_name, isclient=False):
    if not isclient:
        mb.class_(cls_name).add_wrapper_code( type + ' ' + var_name + '_Get() {\r\n' + \
        '   return ' + var_name + '.Get();\r\n' + \
        '}\r\n' )
        mb.class_(cls_name).add_wrapper_code( 'void ' + var_name + '_Set('+type+' val) {\r\n' + \
        '   '+var_name + '.Set(val);\r\n' + \
        '}\r\n' )
    else:
        mb.class_(cls_name).add_wrapper_code( type + ' ' + var_name + '_Get() {\r\n' + \
        '   return ' + var_name + ';\r\n' + \
        '}\r\n' )
        mb.class_(cls_name).add_wrapper_code( 'void ' + var_name + '_Set('+type+' val) {\r\n' + \
        '   '+var_name + ' = val;\r\n' + \
        '}\r\n' )
    mb.class_(cls_name).add_registration_code( 'add_property("'+expose_name+'", \r\n'  + \
    '   &'+cls_name+'_wrapper::'+var_name + '_Get,\r\n'  + \
    '   &'+cls_name+'_wrapper::'+var_name + '_Set )\r\n' 
    )
    
# ---------------------------------------------------------
# Handle expose code
def GenerateEntHandleExposeCode( cls_name, handle_name ):
    code = '{ //::'+handle_name+'\r\n' + \
    '    typedef bp::class_< '+handle_name+', bp::bases< CBaseHandle > > '+handle_name+'_exposer_t;\r\n' + \
    '    '+handle_name+'_exposer_t '+handle_name+'_exposer = '+handle_name+'_exposer_t( "'+handle_name+'", bp::init< >() );\r\n' + \
    '    '+handle_name+'_exposer.def( bp::init< '+cls_name+' * >(( bp::arg("pVal") )) );\r\n' + \
    '    '+handle_name+'_exposer.def( bp::init< int, int >(( bp::arg("iEntry"), bp::arg("iSerialNumber") )) );\r\n' + \
    '    { //::'+handle_name+'::GetAttr\r\n' + \
    '    \r\n' + \
    '        typedef bp::object ( ::'+handle_name+'::*GetAttr_function_type )( const char * ) const;\r\n' + \
    '        \r\n' + \
    '        '+handle_name+'_exposer.def( \r\n' + \
    '            "__getattr__"\r\n' + \
    '            , GetAttr_function_type( &::'+handle_name+'::GetAttr )\r\n' + \
    '        );\r\n' + \
    '    \r\n' + \
    '    }\r\n' + \
    '    { //::'+handle_name+'::Cmp\r\n' + \
    '    \r\n' + \
    '        typedef bool ( ::'+handle_name+'::*Cmp_function_type )( bp::object ) const;\r\n' + \
    '        \r\n' + \
    '        '+handle_name+'_exposer.def( \r\n' + \
    '            "__cmp__"\r\n' + \
    '            , Cmp_function_type( &::'+handle_name+'::Cmp )\r\n' + \
    '        );\r\n' + \
    '    \r\n' + \
    '    }\r\n' + \
    '    { //::'+handle_name+'::NonZero\r\n' + \
    '    \r\n' + \
    '        typedef bool ( ::'+handle_name+'::*NonZero_function_type )( ) const;\r\n' + \
    '        \r\n' + \
    '        '+handle_name+'_exposer.def( \r\n' + \
    '            "__nonzero__"\r\n' + \
    '            , NonZero_function_type( &::'+handle_name+'::NonZero )\r\n' + \
    '        );\r\n' + \
    '    \r\n' + \
    '    }\r\n' + \
    '    { //::'+handle_name+'::Set\r\n' + \
    '    \r\n' + \
    '        typedef void ( ::'+handle_name+'::*Set_function_type )( '+ cls_name +' * ) const;\r\n' + \
    '        \r\n' + \
    '        '+handle_name+'_exposer.def( \r\n' + \
    '            "Set"\r\n' + \
    '            , Set_function_type( &::'+handle_name+'::Set )\r\n' + \
    '        );\r\n' + \
    '    \r\n' + \
    '    }\r\n' + \
    '    { //::'+handle_name+'::GetSerialNumber\r\n' + \
    '    \r\n' + \
    '        typedef int ( ::'+handle_name+'::*GetSerialNumber_function_type )( ) const;\r\n' + \
    '        \r\n' + \
    '        '+handle_name+'_exposer.def( \r\n' + \
    '            "GetSerialNumber"\r\n' + \
    '            , GetSerialNumber_function_type( &::'+handle_name+'::GetSerialNumber )\r\n' + \
    '        );\r\n' + \
    '    \r\n' + \
    '    }\r\n' + \
    '    { //::'+handle_name+'::GetEntryIndex\r\n' + \
    '    \r\n' + \
    '        typedef int ( ::'+handle_name+'::*GetEntryIndex_function_type )(  ) const;\r\n' + \
    '        \r\n' + \
    '        '+handle_name+'_exposer.def( \r\n' + \
    '            "GetEntryIndex"\r\n' + \
    '            , GetEntryIndex_function_type( &::'+handle_name+'::GetEntryIndex )\r\n' + \
    '        );\r\n' + \
    '    \r\n' + \
    '    }\r\n' + \
    '    '+handle_name+'_exposer.def( bp::self != bp::self );\r\n' + \
    '    '+handle_name+'_exposer.def( bp::self == bp::self );\r\n' + \
    '}\r\n'
    return code
 
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

# Converters
def AddEntityConverter( mb, cls_name ):
    cls = mb.class_(cls_name)
    
    handlename = cls_name+'HANDLE'
    
    ptr_convert_to_py_name = 'ptr_'+cls_name+'_to_handle'
    convert_to_py_name = cls_name+'_to_handle'
    convert_from_py_name = 'handle_to_'+cls_name
    
    # Add handle typedef
    mb.add_declaration_code( 'typedef CEPyHandle<'+cls_name+'> '+handlename+';\r\n' )
    
    # Expose handle code
    mb.add_registration_code( GenerateEntHandleExposeCode(cls_name, handlename), True )
    
    # Add to python converters
    mb.add_declaration_code(
    'struct '+ptr_convert_to_py_name+' : bp::to_python_converter<'+cls_name+' *, ptr_'+cls_name+'_to_handle>\r\n' + \
    '{\r\n' + \
    '    static PyObject* convert('+cls_name+' *s)\r\n' + \
    '    {\r\n' + \
    '        return s ? bp::incref(s->GetPyHandle().ptr()) : bp::incref(Py_None);\r\n' + \
    '    }\r\n' + \
    '};\r\n'
    )

    mb.add_declaration_code(
    'struct '+convert_to_py_name+' : bp::to_python_converter<'+cls_name+', '+cls_name+'_to_handle>\r\n' + \
    '{\r\n' + \
    '    static PyObject* convert(const '+cls_name+' &s)\r\n' + \
    '    {\r\n' + \
    '        return bp::incref(s.GetPyHandle().ptr());\r\n' + \
    '    }\r\n' + \
    '};\r\n'
    )

    # Add from python converter
    # NOTE: Not sure if the part where Py_None is returned is correct
    # Returning NULL means it can't be converted. Returning Py_None seems to result in NULL being passed to the function.
    mb.add_declaration_code(
    'struct '+convert_from_py_name + '\r\n' + \
    '{\r\n' + \
    '    handle_to_'+cls_name+'()\r\n' + \
    '    {\r\n' + \
    '        bp::converter::registry::insert(\r\n' + \
    '            &extract_'+cls_name+', \r\n' + \
    '            bp::type_id<'+cls_name+'>()\r\n' + \
    '            );\r\n' + \
    '    }\r\n' + \
    '\r\n' + \
    '    static void* extract_'+cls_name+'(PyObject* op){\r\n' + \
    '       CBaseHandle h = bp::extract<CBaseHandle>(op);\r\n' + \
    '       if( h.Get() == NULL )\r\n' + \
    '           return Py_None;\r\n' + \
    '       return h.Get();\r\n' + \
    '    }\r\n' + \
    '};\r\n'
    )
    
    # Add registration code
    mb.add_registration_code( ptr_convert_to_py_name+"();" )
    mb.add_registration_code( convert_to_py_name+"();" )
    mb.add_registration_code( convert_from_py_name+"();" )

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
    {
        static PyObject* convert('+cls_name+' *s)
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

# Almost the same as above, maybe make a shared function?   
def AddVGUIConverter(mb, cls_name, novguilib, containsabstract=False):
    cls = mb.class_(cls_name)
    
    handlename = cls_name+'HANDLE'
    
    ptr_convert_to_py_name = 'ptr_%(clsname)s_to_handle' % {'clsname' : cls_name}
    convert_to_py_name = '%(clsname)s_to_handle' % {'clsname' : cls_name}
    convert_from_py_name = 'handle_to_%(clsname)s' % {'clsname' : cls_name}
    
    # Arguments for templates
    strargs = {
        'clsname' : cls_name,
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
    
    constructors = cls.constructors(name=cls_name)
    constructors.body = '\tg_PythonPanelCount++;'  
        
    cls.add_wrapper_code( '~%(clsname)s_wrapper( void ) { g_PythonPanelCount--; /*::PyDeletePanel( this, this );*/ }' % strargs ) 
    
    if novguilib:
        cls.add_wrapper_code( 'void DeletePanel( void ) { ::PyDeletePanel( this, this ); }' )
    else:
        cls.add_wrapper_code( 'void DeletePanel( void ) {}' )
    
    # Add handle typedef
    mb.add_declaration_code( 'typedef PyVGUIHandle<%(clsname)s> %(handlename)s;\r\n' % strargs )
    
    # Expose handle code
    mb.add_registration_code(vguihandle_template % {'clsname' : cls_name, 'handlename' : handlename}, True)
    
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
    
# ---------------------------------------------------------
known_classes = [
    # Entities ( src_ents module )
    'C_BaseEntity', 
    'C_BaseAnimating', 
    'C_BaseAnimatingOverlay', 
    'C_BaseFlex', 
    'C_BaseCombatCharacter',
    'C_BasePlayer',
    'C_HL2WarsPlayer',
    
    # Entities ( srcents module )
    'CBaseEntity', 
    'CBaseAnimating', 
    'CBaseAnimatingOverlay', 
    'CBaseFlex', 
    'CBaseCombatCharacter',
    'CBasePlayer',
    'CHL2WarsPlayer',
    
    # Tracing ( srcbase )
    'CBaseTrace',
    'CGameTrace',
    'Ray_t',
    'ITraceFilter',
    'CTraceFilter',
    
    # Physics
    'IPhysicsObject',           # srcbase
    'IPhysicsObjectHandle',     # srcbase
    'CollisionProp',            # srcbase
    
    # Models
    'studiohdr_t',              # srcbase
    
    # Gameinterface
    'CCommand',
    
    # Boost python
    'boost::python::object',
    'boost::python::list',
    'boost::python::tuple'
]

known_enums = [
    # Enums
    'SolidType_t',      # srcbase
    'MoveType_t',       # srcbase
    'MoveCollide_t',    # srcbase
    'RenderMode_t',     # srcbase
]

def DisableKnownWarnings(mb):
    # We know the following classes are exposed in a different module
    if mb.global_ns.classes(name='Color', allow_empty=True):
        mb.classes('Color').disable_warnings( messages.W1040 )          # srcbase module 
        
    if mb.global_ns.classes(name='string_t', allow_empty=True):
        mb.classes('string_t').disable_warnings( messages.W1040 )       # srcbase module 
        
    if mb.global_ns.classes(name='Vector', allow_empty=True):
        mb.classes('Vector').disable_warnings( messages.W1040 )         # srcbase module 
        
    if mb.global_ns.classes(name='QAngle', allow_empty=True):
        mb.classes('QAngle').disable_warnings( messages.W1040 )         # srcbase module 
        
    if mb.global_ns.classes(name='color32_s', allow_empty=True):
        mb.classes('color32_s').disable_warnings( messages.W1040 )      # srcbase module
        
    if mb.global_ns.classes(name='KeyValues', allow_empty=True):
        mb.classes('KeyValues').disable_warnings( messages.W1040 )      # srcbase module
    
    for cls_name in known_classes:
        if mb.global_ns.classes(name=cls_name, allow_empty=True):
            mb.class_(cls_name).disable_warnings( messages.W1040 )     
    
    for enum_name in known_enums:
        if mb.global_ns.enums(name=enum_name, allow_empty=True):
            mb.enum(enum_name).disable_warnings( messages.W1040 )  