LIB_FILES = 		../../src/converter/arg_to_python_base.cpp \
		../../src/converter/builtin_converters.cpp \
		../../src/object/class.cpp \
		../../src/dict.cpp \
		../../src/object/enum.cpp \
		../../src/errors.cpp \
		../../src/exec.cpp \
		../../src/converter/from_python.cpp \
		../../src/object/function.cpp \
		../../src/object/function_doc_signature.cpp \
		../../src/import.cpp \
		../../src/object/inheritance.cpp \
		../../src/object/iterator.cpp \
		../../src/object/life_support.cpp \
		../../src/list.cpp \
		../../src/long.cpp \
		../../src/module.cpp \
		../../src/numeric.cpp \
		../../src/object_operators.cpp \
		../../src/object_protocol.cpp \
		../../src/object/pickle_support.cpp \
		../../src/converter/registry.cpp \
		../../src/slice.cpp \
		../../src/object/stl_iterator.cpp \
		../../src/str.cpp \
		../../src/tuple.cpp \
		../../src/converter/type_id.cpp \
		../../src/wrapper.cpp \

# All source files have associated object files
LIBOFILES		= $(LIB_FILES:%.cpp=%.o) 

CXX = g++-4.3
LDLIBRARY	= bp.so
CFLAGS 		= -DBOOST_AUTO_LINK_NOMANGLE -DBOOST_PYTHON_STATIC_LIB -DPy_NO_ENABLE_SHARED -DBOOST_PYTHON_SOURCE -DNDEBUG

INCL			= -I../../../../ -I../../../../../python/Include -I../../../../../python -I../../../../../public -I ../../../../../public/tier0

AR=		ar
RANLIB=		ranlib

LINKFORSHARED=	-Xlinker -export-dynamic
LDSHARED=	$(CXX) -shared -L../../../../../python/ -lpython2.6
BLDSHARED=	$(CXX) -shared -L../../../../../python/ -lpython2.6
LDFLAGS =  

all: libbp.a

.cpp.o: 
	$(CXX) -c $(CFLAGS) $(INCL) -o $@ $<

libbp.a: $(LIBOFILES)
	-rm -f $@
	$(AR) cr $@ $(LIBOFILES)
	$(RANLIB) $@
	-cp $@ ../../../../../lib/linux/$@

libbp.so: $(LIBOFILES)
	$(LDSHARED) $(LDFLAGS) $(LINKFORSHARED) -o $@ $(LIBOFILES)

clean:
	find ../../src/ -name '*.o' -exec rm -f {} ';'
	find . -name '*.so' -exec rm -f {} ';'
	find . -name '*.a' -exec rm -f {} ';'


