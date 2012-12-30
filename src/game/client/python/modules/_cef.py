from generate_mods_helper import GenerateModuleClient
from src_helper import *
import settings

from pyplusplus import function_transformers as FT
from pygccxml.declarations import matchers
from pyplusplus.module_builder import call_policies
from pyplusplus import code_creators

class CEF(GenerateModuleClient):
    module_name = '_cef'

    files = [
        'videocfg/videocfg.h',
        
        'cbase.h',
        
        'src_cef_browser.h',
        'src_cef_python.h',
    ]
    
    def Parse(self, mb):
        # Exclude everything by default
        mb.decls().exclude()  
        
        cls = mb.class_('SrcCefBrowser')
        cls.include()
        
        # Excludes
        cls.mem_fun('GetPanel').exclude()
        cls.mem_fun('GetOSRHandler').exclude()
        cls.mem_fun('GetBrowser').exclude()
        #cls.mem_fun('GetClientHandler').exclude()
        
        cls.mem_fun('PyGetMainFrame').rename('GetMainFrame')
        
        cls.mem_fun('OnLoadStart').exclude()
        cls.mem_fun('OnLoadEnd').exclude()
        cls.mem_fun('OnLoadError').exclude()
        cls.mem_fun('CreateGlobalObject').exclude()
        cls.mem_fun('OnMethodCall').exclude()
        
        cls.mem_fun('PyOnLoadStart').rename('OnLoadStart')
        cls.mem_fun('PyOnLoadEnd').rename('OnLoadEnd')
        cls.mem_fun('PyOnLoadError').rename('OnLoadError')
        cls.mem_fun('PyCreateGlobalObject').rename('CreateGlobalObject')
        cls.mem_fun('PyOnMethodCall').rename('OnMethodCall')
        
        cls = mb.class_('PyJSObject')
        cls.include()
        cls.rename('JSObject')
        
        
        '''
        cls = mb.class_('PyCefV8Value')
        cls.include()
        cls.rename('CefV8Value')
        
        cls.mem_fun('GetAttr').rename('__getattr__')
        cls.mem_fun('GetAttribute').rename('__getattribute__')
        cls.mem_fun('SetAttr').rename('__setattr__')
        
        cls = mb.class_('PyCefV8Context')
        cls.include()
        cls.rename('CefV8Context')
        cls.no_init = True
        
        cls.mem_fun('Enter').rename('__enter__')
        cls.mem_fun('Exit').rename('__exit__')
        '''
        
        cls = mb.class_('PyCefFrame')
        cls.include()
        cls.rename('CefFrame')
        cls.no_init = True
        
    def AddAdditionalCode(self, mb):
        header = code_creators.include_t( 'include/cef_v8.h' )
        mb.code_creator.adopt_include(header)
        header = code_creators.include_t( 'include/cef_process_message.h' )
        mb.code_creator.adopt_include(header)
        header = code_creators.include_t( 'src_cef_js.h' )
        mb.code_creator.adopt_include(header)
        super(CEF, self).AddAdditionalCode(mb)
        