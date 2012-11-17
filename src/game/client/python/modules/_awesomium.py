from generate_mods_helper import GenerateModuleClient
from src_helper import *
import settings

from pyplusplus import function_transformers as FT
from pygccxml.declarations import matchers
from pyplusplus.module_builder import call_policies
from pyplusplus import code_creators

class Awesomium(GenerateModuleClient):
    module_name = '_awesomium'
    
    if settings.ASW_CODE_BASE:
        files = ['videocfg/videocfg.h']
    else:
        files = ['wchartypes.h', 'shake.h']
        
    files.extend( [
        'cbase.h',
        
        #'Awesomium/WebCore.h',
        'hl2wars/vgui/vgui_webview.h',
    ] )

    def Parse(self, mb):
        # Exclude everything by default
        mb.decls().exclude()  

        cls = mb.class_('::WebView')
        cls.include()
        cls.mem_funs('GetWebViewInternal').exclude()
        cls.mem_funs('GetPanel').exclude()
        cls.mem_funs('GetWebView').exclude()
        
        cls.mem_funs('OnChangeTitle').exclude()
        cls.mem_funs('OnChangeAddressBar').exclude()
        cls.mem_funs('OnChangeTooltip').exclude()
        cls.mem_funs('OnChangeTargetURL').exclude()
        cls.mem_funs('OnChangeCursor').exclude()
        cls.mem_funs('OnShowCreatedWebView').exclude()
        
        cls.mem_funs('OnBeginLoadingFrame').exclude()
        cls.mem_funs('OnFailLoadingFrame').exclude()
        cls.mem_funs('OnFinishLoadingFrame').exclude()
        cls.mem_funs('OnDocumentReady').exclude()
        
        cls.mem_funs('OnUnresponsive').exclude()
        cls.mem_funs('OnResponsive').exclude()
        cls.mem_funs('OnCrashed').exclude()
        
        cls.mem_funs('OnMethodCall').exclude()
        cls.mem_funs('PyOnMethodCall').rename('OnMethodCall')
        cls.mem_funs('OnMethodCallWithReturnValue').exclude()
        
        cls = mb.class_('PyJSObject')
        cls.include()
        cls.rename('JSObject')
        
        # Enums
        mb.enum('Cursor').include()
        mb.enum('FocusedElementType').include()
        mb.enum('TerminationStatus').include()
        
    def AddAdditionalCode(self, mb):
        header = code_creators.include_t( 'Awesomium/WebCore.h' )
        mb.code_creator.adopt_include(header)
        super(Awesomium, self).AddAdditionalCode(mb)
        