import os

from . basegenerator import ModuleGenerator
from .. src_module_builder import src_module_builder_t
from .. matchers import MatcherTestInheritClass
from pyplusplus.module_builder import call_policies
from pygccxml.declarations import matchers, pointer_t, reference_t, const_t, declarated_t, void_t, compound_t

class SourceModuleGenerator(ModuleGenerator):
    # Choices: client, server, semi_shared and pure_shared
    # semi_shared: the module is parsed two times, with different preprocessors.
    #              The output will be added in the same file between #ifdef CLIENT_DLL #else #endif
    # pure_shared: The generated bindings must match on the client and server.
    module_type = 'client'
    
    # Set by generation code
    isclient = False
    isserver = False
    
    @property
    def srcdir(self):
        return self.clientsrcdir if self.isclient else self.serversrcdir
        
    @property
    def vpcdir(self):
        return self.clientvpcdir if self.isclient else self.servervpcdir
    
    @property
    def includes(self):
        return self.clientincludes if self.isclient else self.serverincludes
            
    @property
    def symbols(self):
        return self.clientsymbols if self.isclient else self.serversymbols
        
    @property
    def path(self):
        return os.path.join('../../', self.settings.client_path)
    
    # Create builder
    def CreateBuilder(self, files, parseonlyfiles):
        os.chdir(self.vpcdir)
        mb = src_module_builder_t(files, self.includes, self.symbols, is_client=self.isclient)
        mb.parseonlyfiles = parseonlyfiles
        return mb
        
    def GetFilenames(self):
        path = rm.path
        if not rm.split:
            outfilename = '%s.cpp' % (rm.module_name)
            return [os.path.relpath(os.path.join(path, outfilename), os.path.join(rm.servervpcdir, rm.serversrcpath))]
        else:
            files = os.listdir(os.path.join(rm.path, rm.module_name))
            files = filter(lambda f: f.endswith('.cpp') or f.endswith('.hpp'), files)
            return map(lambda f: os.path.join(path, rm.module_name, f), files)
    
    def GetFiles(self):
        parsefiles = list(self.files)
        
        # Only for parsing, seems required for some reason (or some error somewhere in the gccxml setup?)
        if self.settings.branch == 'swarm':
            parsefiles.insert(0, '$%videocfg/videocfg.h')
    
        parseonlyfiles = []
        files = []
        for filename in parsefiles:
            if not filename:
                continue
                
            addtoserver = False
            addtoclient = False
            parseonly = False
            while filename[0] in ['#', '$', '%']:
                c = filename[0]
                if c == '#':
                    addtoserver = True 
                elif c == '$':
                    addtoclient = True
                elif c == '%':
                    parseonly = True
                filename = filename[1:]
                
            if not addtoserver and not addtoclient:
                files.append(filename)
            if self.isserver and addtoserver:
                files.append(filename)
            if self.isclient and addtoclient:
                files.append(filename)
                
            if parseonly and filename in files:
                parseonlyfiles.append(filename)
                
        return files, parseonlyfiles
        
    def PostCodeCreation(self, mb):
        ''' Allows modifying mb.code_creator just after the code creation. '''
        parseonlyfiles = list(mb.parseonlyfiles)
        
        # Remove boost\python.hpp header. This is already included by srcpy.h
        # and directly including can break debug mode (because it redefines _DEBUG)
        parseonlyfiles.append(r'boost/python.hpp')

        # Remove files which where added for parsing only
        for filename in parseonlyfiles:
            found = False
            testfilenamepath = os.path.normpath(filename)
            for creator in mb.code_creator.creators:
                try:
                    testpath = os.path.normpath(creator.header)
                    if testfilenamepath == testpath:
                        found = True
                        mb.code_creator.remove_creator(creator)
                        break
                except:
                    pass
            if not found:
                raise Exception('Could not find %s header''' % (filename))
        
    def TestCBaseEntity(self, cls):
        baseentcls = self.baseentcls
        recursive_bases = cls.recursive_bases
        for testcls in recursive_bases:
            if baseentcls == testcls.related_class:
                return True
        return cls == baseentcls
        
    def AddNetworkVarProperty(self, exposename, varname, ctype, clsname):
        cls = self.mb.class_(clsname) if (type(clsname) == str) else clsname
        args = {
            'varname' : varname,
            'type' : ctype,
            'exposename' : exposename,
            'clsname' : cls.name,
        }
        if not self.isclient:
            cls.add_wrapper_code('static %(type)s %(varname)s_Get( %(clsname)s const & inst ) { return inst.%(varname)s.Get(); }' % args)
            cls.add_wrapper_code('static void %(varname)s_Set( %(clsname)s & inst, %(type)s val ) { inst.%(varname)s.Set( val ); }' % args)
        else:
            cls.add_wrapper_code('static %(type)s %(varname)s_Get( %(clsname)s const & inst ) { return inst.%(varname)s; }' % args)
            cls.add_wrapper_code('static void %(varname)s_Set( %(clsname)s & inst, %(type)s val ) { inst.%(varname)s = val; }' % args)
        cls.add_registration_code('add_property( "%(exposename)s", &%(clsname)s_wrapper::%(varname)s_Get, &%(clsname)s_wrapper::%(varname)s_Set )' % (args))
            
    # Applies common rules to code
    def ApplyCommonRules(self, mb):
        # Common function added for getting the "PyObject" of an entity
        mb.mem_funs('GetPySelf').exclude()
        
        # All return values derived from IHandleEntity entity will be returned by value.
        # This ensures the converter is called
        testinherit = MatcherTestInheritClass(mb.class_('IHandleEntity'))
        decls = mb.calldefs(matchers.custom_matcher_t(testinherit))
        decls.call_policies = call_policies.return_value_policy(call_policies.return_by_value)
        
        # All CBaseEntity related classes should have a custom call trait
        self.baseentcls = mb.class_('CBaseEntity' if self.isserver else 'C_BaseEntity')
        def ent_call_trait(type_):
            return '%(arg)s ? %(arg)s->GetPyHandle() : boost::python::object()'
        entclasses = mb.classes(self.TestCBaseEntity)
        for entcls in entclasses:
            entcls.custom_call_trait = ent_call_trait
        
        # Anything returning KeyValues should be returned by value so it calls the converter
        keyvalues = mb.class_('KeyValues')
        mb.calldefs(matchers.calldef_matcher_t(return_type=pointer_t(declarated_t(keyvalues))), allow_empty=True).call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
        mb.calldefs(matchers.calldef_matcher_t(return_type=pointer_t(const_t(declarated_t(keyvalues)))), allow_empty=True).call_policies = call_policies.return_value_policy(call_policies.return_by_value) 
        
        # Anything returning a void pointer is excluded by default
        mb.calldefs(matchers.calldef_matcher_t(return_type=pointer_t(declarated_t(void_t()))), allow_empty=True).exclude()
        mb.calldefs(matchers.calldef_matcher_t(return_type=pointer_t(const_t(declarated_t(void_t())))), allow_empty=True).exclude()
        
