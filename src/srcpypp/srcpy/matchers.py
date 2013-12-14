from pygccxml.declarations import matchers, cpptypes, compound_t, declarated_t

class calldef_withtypes(matchers.custom_matcher_t):
    def __init__(self, matchtypes=None):
        super(calldef_withtypes, self).__init__(self.testdecl)
        
        self.matchtypes = matchtypes
        
    def __compare_types( self, type_or_str, type ):
        assert type_or_str
        if type is None:
            return False
        if isinstance( type_or_str, cpptypes.type_t ):
            if type_or_str != type:
                return False
        else:
            if type_or_str != type.decl_string:
                return False
        return True
        
    def testdecl( self, decl ):
        if self.matchtypes:
            if isinstance(self.matchtypes, (list, tuple)):
                matchtypes = self.matchtypes
            else:
                matchtypes = [self.matchtypes]
                
            for t in matchtypes:
                if self.__compare_types( t, decl.return_type ):
                    return True
        
                for arg in decl.arguments:
                    if self.__compare_types( t, arg.type ):
                        return True
        
        return False
        
class MatcherTestInheritClass(object):
    def __init__(self, cls):
        super(MatcherTestInheritClass, self).__init__()
        
        self.cls = cls
        
    def __call__(self, decl):
        testinheritcls = self.cls
        
        # Only consider declarated and compound types
        return_type = decl.return_type
        if type(return_type) != declarated_t and not isinstance(return_type, compound_t):
            return False
            
        # Traverse bases of return type
        declaration = None
        while return_type:
            if type(return_type) == declarated_t:
                declaration = return_type.declaration
                break
            if not isinstance(return_type, compound_t):
                break
            return_type = return_type.base
            
        if not declaration:
            return False
            
        if declaration == testinheritcls:
            return True
            
        if hasattr(declaration, 'recursive_bases'):
            # Look through all bases of the class we are testing
            recursive_bases = declaration.recursive_bases
            for testcls in recursive_bases:
                if testinheritcls == testcls.related_class:
                    return True
            
        return False