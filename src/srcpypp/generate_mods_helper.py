#! /usr/bin/python
import sys, os
from src_module_builder import src_module_builder_t
from pyplusplus import code_creators, file_writers
import time 
import settings
import vcxprojupdate

# Append paths
sys.path.append(settings.mod_client_path)
sys.path.append(settings.mod_server_path)
sys.path.append(settings.mod_shared_path)

# Ensure output paths exist
if not os.path.exists(settings.client_path):
    os.mkdir(settings.client_path)
if not os.path.exists(settings.server_path):
    os.mkdir(settings.server_path)
if not os.path.exists(settings.shared_path):
    os.mkdir(settings.shared_path)
    
# List of modules we should generate
registered_modules =  None

def RegisterModules():
    global registered_modules
    
    
    if settings.ASW_CODE_BASE:
        add_mods = settings.modules_asw
    else:
        add_mods = settings.modules_2007
    
    registered_modules = []
    for am in add_mods:
        mod = __import__(am[0])
        cls = getattr(mod, am[1])
        registered_modules.append( cls() )

#
# Classes
#
def unusedfile_f(file):
    print('Unused file: %s' % (file) )
    
class GenerateModule(object):
    # Settings
    module_name = 'the_name_of_this_module'
    split = False   # Can't be true if module_type == 'semi_shared'
    
    # Choices: client, server, semi_shared and pure_shared
    # semi_shared: the module is parsed two times, with different preprocessors.
    #              The output will be added in the same file between #ifdef CLIENT_DLL #else #endif
    # pure_shared: The generated bindings must match on the client and server.
    module_type = 'client'  
    
    files = []
    
    # Map some names
    dll_name = None
    path = None
    
    # Init, check.
    def __init__(self):
        super(GenerateModule, self).__init__()
        
        self.isClient = False

    # Main method
    def Run(self):
        mb = self.CreateBuilder(self.files)
        self.Parse(mb)
        self.FinalOutput(mb)
        
    # Parse method. Implement this.
    def Parse(self, mb):
        assert(0)
        
    # Create builder
    def CreateBuilder(self, files):
        return src_module_builder_t(files, is_client=False)
            
    def FinalOutput(self, mb):
        ''' Finalizes the output after generation of the bindings.
            Writes output to file.'''
        # Set pydocstring options
        mb.add_registration_code('bp::docstring_options doc_options( true, true, false );', tail=False)
    
        # Generate code
        mb.build_code_creator(module_name=self.module_name)      

        # Add precompiled header + other general required stuff and write away
        self.AddAdditionalCode(mb)      
        if self.split:
            written_files = mb.split_module(os.path.join(self.path, self.module_name), on_unused_file_found=unusedfile_f)
        else:
            mb.write_module(os.path.join(os.path.abspath(self.path), self.module_name+'.cpp'))
        
    # Adds precompiled header + other default includes
    def AddAdditionalCode(self, mb):
        mb.code_creator.user_defined_directories.append( os.path.abspath('.') )
        header = code_creators.include_t( 'cbase.h' )
        mb.code_creator.adopt_creator( header, 0 )
        header = code_creators.include_t( 'src_python.h' )
        mb.code_creator.adopt_include( header )
        header = code_creators.include_t( 'tier0/memdbgon.h' )
        mb.code_creator.adopt_include(header)
        
    def AddProperty(self, cls, propertyname, getter, setter=''):
        cls.mem_funs(getter).exclude()
        if setter: cls.mem_funs(setter).exclude()
        if setter:
            cls.add_property(propertyname, cls.member_function( getter ), cls.member_function( setter ))
        else:
            cls.add_property(propertyname, cls.member_function( getter ))
                             
class GenerateModuleClient(GenerateModule):
    module_type = 'client'
    dll_name = 'Client'
    path = settings.client_path
    
    # Create builder
    def CreateBuilder(self, files):
        return src_module_builder_t(files, is_client=True)    
    
class GenerateModuleServer(GenerateModule):
    module_type = 'server'
    dll_name = 'Server'
    path = settings.server_path
    
class GenerateModulePureShared(GenerateModule):
    module_type = 'pure_shared'
    dll_name = 'Shared'
    path = settings.shared_path
    
 
class multiple_files_nowrite_t(file_writers.multiple_files_t): 
    def __init__(self, extmodule, directory_path, write_main=True, files_sum_repository=None, encoding='ascii'):
        super(multiple_files_nowrite_t, self).__init__(extmodule, directory_path, write_main, files_sum_repository, encoding)
        
        self.content = {}
        
    #@staticmethod
    def write_file( self, fpath, content, files_sum_repository=None, encoding='ascii' ):
        """Write a source file.

        This method writes the string content into the specified file.
        An additional fixed header is written at the top of the file before
        content.

        :param fpath: File name
        :type fpath: str
        :param content: The content of the file
        :type content: str
        """
        fname = os.path.split( fpath )[1]
        file_writers.writer.writer_t.logger.debug( 'write code to file "%s" - started' % fpath )
        start_time = time.clock()
        fcontent_new = []
        if os.path.splitext( fpath )[1] == '.py':
            fcontent_new.append( '# This file has been generated by Py++.' )
        else:
            fcontent_new.append( '// This file has been generated by Py++.' )
        fcontent_new.append( os.linesep * 2 )
        fcontent_new.append( content )
        fcontent_new.append( os.linesep ) #keep gcc happy
        fcontent_new = ''.join( fcontent_new )
        if not isinstance( fcontent_new, unicode ):
            fcontent_new = unicode( fcontent_new, encoding )

        new_hash_value = None
        curr_hash_value = None
        if files_sum_repository:
            new_hash_value  = files_sum_repository.get_text_value( fcontent_new )
            curr_hash_value = files_sum_repository.get_file_value( fname )
            if new_hash_value == curr_hash_value:
                file_writers.writer.writer_t.logger.debug( 'file was not changed( hash ) - done( %f seconds )'
                                       % ( time.clock() - start_time ) )
                return
                
        self.content[fpath] = fcontent_new
        
        if new_hash_value:
            files_sum_repository.update_value( fname, new_hash_value )
        file_writers.writer.writer_t.logger.info( 'file "%s" - updated( %f seconds )' % ( fname, time.clock() - start_time ) )

class class_multiple_files_nowrite_t(file_writers.class_multiple_files_t): 
    def __init__(self
                  , extmodule
                  , directory_path
                  , huge_classes
                  , num_of_functions_per_file=20
                  , files_sum_repository=None
                  , encoding='ascii'):
        super(class_multiple_files_nowrite_t, self).__init__(extmodule, directory_path,
                huge_classes, num_of_functions_per_file, files_sum_repository, encoding)
        
        self.content = {}
        
    #@staticmethod
    def write_file( self, fpath, content, files_sum_repository=None, encoding='ascii' ):
        """Write a source file.

        This method writes the string content into the specified file.
        An additional fixed header is written at the top of the file before
        content.

        :param fpath: File name
        :type fpath: str
        :param content: The content of the file
        :type content: str
        """
        fname = os.path.split( fpath )[1]
        file_writers.writer.writer_t.logger.debug( 'write code to file "%s" - started' % fpath )
        start_time = time.clock()
        fcontent_new = []
        if os.path.splitext( fpath )[1] == '.py':
            fcontent_new.append( '# This file has been generated by Py++.' )
        else:
            fcontent_new.append( '// This file has been generated by Py++.' )
        fcontent_new.append( os.linesep * 2 )
        fcontent_new.append( content )
        fcontent_new.append( os.linesep ) #keep gcc happy
        fcontent_new = ''.join( fcontent_new )
        if not isinstance( fcontent_new, unicode ):
            fcontent_new = unicode( fcontent_new, encoding )

        new_hash_value = None
        curr_hash_value = None
        if files_sum_repository:
            new_hash_value  = files_sum_repository.get_text_value( fcontent_new )
            curr_hash_value = files_sum_repository.get_file_value( fname )
            if new_hash_value == curr_hash_value:
                file_writers.writer.writer_t.logger.debug( 'file was not changed( hash ) - done( %f seconds )'
                                       % ( time.clock() - start_time ) )
                return
                
        self.content[fpath] = fcontent_new
        
        if new_hash_value:
            files_sum_repository.update_value( fname, new_hash_value )
        file_writers.writer.writer_t.logger.info( 'file "%s" - updated( %f seconds )' % ( fname, time.clock() - start_time ) )

        
class GenerateModuleSemiShared(GenerateModule):
    module_type = 'semi_shared'
    dll_name = 'Shared'
    #path = settings.shared_path
    @property
    def path(self):
        if not self.split:
            return settings.shared_path # Into one file with #ifdefs around the different parts
        else:
            if self.isClient:
                return settings.client_path
            return settings.server_path
    
    client_huge_classes = None
    server_huge_classes = None
    
    def GetFiles(self):
        return self.files
    
    # Main method
    def Run(self):
        self.isClient = True
        self.isServer = False
        mb_client = self.CreateBuilder(self.GetFiles())
        self.Parse(mb_client)
        self.isClient = False
        self.isServer = True
        mb_server = self.CreateBuilder(self.GetFiles())
        self.Parse(mb_server)
        self.FinalOutput(mb_client, mb_server)
        
    # Create builder
    def CreateBuilder(self, files):
        if self.isClient:
            return src_module_builder_t(files, is_client=True)   
        return src_module_builder_t(files, is_client=False)   
        
    def GenerateContent(self, mb):
        return mb.get_module()
        
    # Default includes
    def AddAdditionalCode(self, mb):
        mb.code_creator.user_defined_directories.append( os.path.abspath('.') )
        header = code_creators.include_t( 'src_python.h' )
        mb.code_creator.adopt_include( header )
        header = code_creators.include_t( 'tier0/memdbgon.h' )
        mb.code_creator.adopt_include(header)

    def FinalOutput(self, mb_client, mb_server):
        # Set pydocstring options
        mb_client.add_registration_code('bp::docstring_options doc_options( true, true, false );', tail=False)
        mb_server.add_registration_code('bp::docstring_options doc_options( true, true, false );', tail=False)
        
        # Generate code
        mb_client.build_code_creator( module_name=self.module_name )  
        mb_server.build_code_creator( module_name=self.module_name )     
        
        # Misc
        self.isClient = True
        self.isServer = False
        self.AddAdditionalCode(mb_client)   
        self.isClient = False
        self.isServer = True
        self.AddAdditionalCode(mb_server)   
        
        if not self.split:
            content_client = self.GenerateContent(mb_client)
            content_server = self.GenerateContent(mb_server)
            
            content =  '#include "cbase.h"\r\n' + \
                       '\r\n#ifdef CLIENT_DLL\r\n\r\n' + \
                       content_client + \
                       '\r\n\r\n#else\r\n\r\n' + \
                       content_server + \
                       '\r\n\r\n#endif // CLIENT_DLL\r\n\r\n'
            
            # Write output
            target_dir = self.path
            file_writers.write_file(os.path.join(target_dir, self.module_name+'.cpp' ), content)
        else:    
            mb_client.merge_user_code()
            mb_server.merge_user_code()
            
            # Write client
            self.isClient = True
            self.isServer = False
            target_dir = os.path.join(self.path, self.module_name)
            if not os.path.isdir(target_dir):
                os.mkdir(target_dir)
                
            if not self.client_huge_classes:
                mfs_client = multiple_files_nowrite_t( mb_client.code_creator, target_dir, files_sum_repository=None, encoding='ascii' )
            else:
                mfs_client = class_multiple_files_nowrite_t( mb_client.code_creator, target_dir, self.client_huge_classes, files_sum_repository=None, encoding='ascii' )
            mfs_client.write()
            
            for key in mfs_client.content.keys():
                content = '#include "cbase.h"\r\n' + \
                          mfs_client.content[key]
                file_writers.write_file(key, content) 
            
            # Write server
            self.isClient = False
            self.isServer = True
            target_dir = os.path.join(self.path, self.module_name)
            if not os.path.isdir(target_dir):
                os.mkdir(target_dir)
                
            if not self.server_huge_classes:
                mfs_server = multiple_files_nowrite_t( mb_server.code_creator, target_dir, files_sum_repository=None, encoding='ascii' )
            else:
                mfs_server = class_multiple_files_nowrite_t( mb_server.code_creator, target_dir, self.server_huge_classes, files_sum_repository=None, encoding='ascii' )
            mfs_server.write()
            
            for key in mfs_server.content.keys():
                content = '#include "cbase.h"\r\n' + \
                          mfs_server.content[key]
                file_writers.write_file(key, content) 

#
# Append code
#        
def CapitalizeFirstLetter( s ):
    return s[0].capitalize() + s[1:len(s)]

# Generate append code
def GenerateAppendCode( f, module_names, dll_name ):
    f.write('//=============================================================================//\n')
    f.write('// This file is automatically generated. CHANGES WILL BE LOST.\n')
    f.write('//=============================================================================//\n\n')
    f.write('#include "cbase.h"\n')
    f.write('#include "src_python.h"\n\n')
    f.write('// memdbgon must be the last include file in a .cpp file!!!\n')
    f.write('#include "tier0/memdbgon.h"\n\n')
    f.write('using namespace boost::python;\n\n')
    f.write('// The init method is in one of the generated files declared\n')
    f.write('#ifdef _WIN32\n')
    for name in module_names:
        f.write('extern "C" __declspec(dllexport) void init'+name+'();\n')
    f.write('#else\n')
    for name in module_names:
        f.write('extern "C"  void init'+name+'();\n')
    f.write('#endif\n')
    f.write('\n// The append function\n')
    f.write( 'void ' + 'Append'+CapitalizeFirstLetter(dll_name)+'Modules'+ '()\n' )
    f.write('{\n')
    for name in module_names:
        f.write('\tAPPEND_MODULE('+ name +')\n')
    f.write('}\n\n')
    
# Generate pydoc code
def GeneratePydocCode( f, module_names, dll_name ):
    if dll_name == 'shared':
        f.write("#ifndef CLIENT_DLL\n")
        
    # Setup command
    f.write('CON_COMMAND( '+'pydoc_gen_%s, "Generates html pages for each hl2 builtin module")\n' % (dll_name))   
    f.write('{\n')
    f.write('\ttry {\n')
    
    # Get pydoc
    f.write('\t\tSrcPySystem()->Run("import pydoc");\n')
    
    # Import all modules
    for name in module_names:
        f.write('\t\tSrcPySystem()->Run("import %s");\n' % (name))
    f.write('\n')
    
    # Verify the docs/dll_name path exists
    f.write('\t\tSrcPySystem()->Run("import os");\n')
    f.write('\t\tSrcPySystem()->Run("import os.path");\n')
    f.write('\t\tSrcPySystem()->Run("if not os.path.exists(\\"docs\\\"):\\n\\tos.mkdir(\\"docs\\")");\n' )
    f.write('\t\tSrcPySystem()->Run("if not os.path.exists(\\"docs\\\\%s\\"):\\n\\tos.mkdir(\\"docs\\\\%s\\")");\n' % (dll_name, dll_name) )
    
    # Iter module names
    for name in module_names:
        # Generate html page
        #f.write('\t\ttuple rs = resolve("'+name+'", 0);\n')   
        f.write('\t\tSrcPySystem()->Run("obj, name = pydoc.resolve(\\"%s\\", 0)");\n' % (name) )
        f.write('\t\tSrcPySystem()->Run("page = pydoc.html.page(pydoc.describe(obj), pydoc.html.document(obj, name))");\n')
        
        # Write to file
        f.write('\t\tSrcPySystem()->Run("fh = open(\\"docs\\\\\\\\%s\\\\\\\\%s.html\\", \\"w+\\")");\n' % (dll_name, name) )
        f.write('\t\tSrcPySystem()->Run("fh.write(page)");\n')
        f.write('\t\tSrcPySystem()->Run("fh.close()");\n')
        f.write('\t\tMsg("Wrote docs\\\\%s.html\\n", \"'+dll_name+'\\\\'+name+'\");\n')
    
    # Catch errors + finish
    f.write('\t} catch(...) {\n')
    f.write('\t\tPyErr_Print();\n')
    f.write('\t}\n')
    f.write('}\n')
    
    if dll_name == 'shared':
        f.write("#endif // CLIENT_DLL\n")
        
        
def GetIncludePath(path):
    # TODO: do this properly
    if path.startswith('..\game\shared'):
        return path.replace('..\game', '..') 
    elif path.startswith('..\game\server\\'):
        return path.replace('..\game\server\\', '') 
    elif path.startswith('..\game\client\\'):
        return path.replace('..\game\client\\', '')
    return path
        
def GetFilenames(rm, isclient=False):
    rm.isClient = isclient
    rm.isServer = not isclient
    
    path = GetIncludePath(rm.path)
    
    if not rm.split:
        return [os.path.join(path, '%s.cpp' % (rm.module_name))]
    else:
        files = os.listdir(os.path.join(rm.path, rm.module_name))
        files = filter(lambda f: f.endswith('.cpp'), files)
        return map(lambda f: os.path.join(path, rm.module_name, f), files)
#
# Parse function
#
def ParseModules():
    global registered_modules
    
    # Keep a list of the append names
    client_modules = []
    server_modules = []
    shared_modules = []
    
    # Keep a list of the filenames
    client_filenames = []
    server_filenames = []
    shared_filenames = []
    
    RegisterModules()
    
    # Parse
    for rm in registered_modules:
        # Check if we should parse this module
        if not settings.APPEND_FILE_ONLY and (not settings.SPECIFIC_MODULE or settings.SPECIFIC_MODULE == rm.module_name):
            # Parse
            print('Generating %s...' % (rm.module_name))
            rs = rm.Run()
    
        # Builde module list
        if rm.module_type == 'client':
            client_modules.append(rm.module_name)
            client_filenames.extend(GetFilenames(rm))
        elif rm.module_type == 'server':
            server_modules.append(rm.module_name)
            server_filenames.extend(GetFilenames(rm))
        else:
            shared_modules.append(rm.module_name)
            if rm.split:
                client_filenames.extend(GetFilenames(rm, isclient=True))
                server_filenames.extend(GetFilenames(rm, isclient=False))
            else:
                shared_filenames.extend(GetFilenames(rm))

    # Generate new append code if needed
    clientappendfile = GenerateAppendFile(settings.client_path, client_modules, 'client')
    serverappendfile = GenerateAppendFile(settings.server_path, server_modules, 'server')
    sharedappendfile = GenerateAppendFile(settings.shared_path, shared_modules, 'shared')
    
    if settings.addpythonfiles:
        client_filenames.extend(map(os.path.normpath, settings.pythonfiles_client))
        server_filenames.extend(map(os.path.normpath, settings.pythonfiles_server))
        shared_filenames.extend(map(os.path.normpath, settings.pythonfiles_shared))
    
    if client_filenames: client_filenames.append(GetIncludePath(clientappendfile))
    if server_filenames: server_filenames.append(GetIncludePath(serverappendfile))
    if shared_filenames: shared_filenames.append(GetIncludePath(sharedappendfile))
    
    if settings.autoupdatevxproj:
        # Server
        allserverfilenames = shared_filenames+server_filenames
        vcxprojupdate.UpdateChanged(allserverfilenames, settings.vcxprojserver)
        
        # Client
        allclientfilenames = shared_filenames+client_filenames
        vcxprojupdate.UpdateChanged(allclientfilenames, settings.vcxprojclient)
    
def GenerateAppendFile(path, module_names, dll_name):
    filename = 'src_append_%s.cpp' % (dll_name)
    path = os.path.join(path, filename)
    f = open(path, 'w+')
    GenerateAppendCode(f, module_names, dll_name)
    f.close()  
    return path
    
