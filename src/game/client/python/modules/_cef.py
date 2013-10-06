from srcpy.module_generators import ClientModuleGenerator

from pyplusplus import function_transformers as FT
from pygccxml.declarations import matchers
from pyplusplus.module_builder import call_policies
from pyplusplus import code_creators

class CEF(ClientModuleGenerator):
    module_name = '_cef'

    files = [
        'cbase.h',
        
        'src_cef.h',
        'src_cef_browser.h',
        'src_cef_python.h',
    ]
    
    def Parse(self, mb):
        # Exclude everything by default
        mb.decls().exclude()  
        
        # Cef System
        cls = mb.class_('CCefSystem')
        cls.include()
        cls.mem_funs().virtuality = 'not virtual'
        cls.no_init = True
        
        cls.mem_funs().exclude()
        cls.mem_funs('SetFocus').include()
        
        mb.free_function('CEFSystem').include()
        mb.free_function('CEFSystem').call_policies = call_policies.return_value_policy( call_policies.reference_existing_object )
       
        # Cef browser
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
        cls.mem_fun('CreateFunction').exclude()
        cls.mem_fun('ExecuteJavaScriptWithResult').exclude()
        cls.mem_fun('OnMethodCall').exclude()
        cls.mem_fun('SendCallback').exclude()
        cls.mem_fun('Invoke').exclude()
        cls.mem_fun('InvokeWithResult').exclude()
        
        cls.mem_fun('PyOnLoadStart').rename('OnLoadStart')
        cls.mem_fun('PyOnLoadEnd').rename('OnLoadEnd')
        cls.mem_fun('PyOnLoadError').rename('OnLoadError')
        cls.mem_fun('PyCreateGlobalObject').rename('CreateGlobalObject')
        cls.mem_fun('PyCreateFunction').rename('CreateFunction')
        cls.mem_fun('PyOnMethodCall').rename('OnMethodCall')
        cls.mem_fun('PyExecuteJavaScriptWithResult').rename('ExecuteJavaScriptWithResult')
        cls.mem_fun('PySendCallback').rename('SendCallback')
        cls.mem_fun('PyInvoke').rename('Invoke')
        cls.mem_fun('PyInvokeWithResult').rename('InvokeWithResult')
        
        cls = mb.class_('PyJSObject')
        cls.include()
        cls.no_init = True
        cls.rename('JSObject')
        cls.mem_fun('GetJSObject').exclude()
        cls.mem_fun('GetIdentifier').exclude()
        cls.mem_fun('GetName').exclude()
        
        cls.add_property( 'identifier'
                         , cls.mem_fun('GetIdentifier') )
        cls.add_property( 'name'
                         , cls.mem_fun('GetName') )
        
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
        