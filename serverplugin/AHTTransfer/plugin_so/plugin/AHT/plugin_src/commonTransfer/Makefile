#OS=`uname``uname -a | sed -n '1p' | perl -nle 'print $$1 if /\s+([0-9]\.\d+)/'`
#GCC=`gcc --version | sed -n '1p' | perl -nle 'print $$1 if /\s+([0-9]\.\d+)/'`
OS=$(shell uname)$(shell uname -a | sed -n '1p' | perl -nle 'print $$1 if /\s+([0-9]\.\d+)/')
GCC=$(shell gcc --version | sed -n '1p' | perl -nle 'print $$1 if /\s+([0-9]\.\d+)/')
CC=g++
VER=1.0.3
COMLIB=/usr/local/avenue/sos/release/
DIR_LIST=$(COMLIB) ./conf ./plugin
OutPut=build
NEW_CODE_PATH=./plugin ./conf
#SOURCE CODE
CC_SRC=$(shell find  $(DIR_LIST)   -name "*.cpp" )
CC_SRC2=$(shell find  $(DIR_LIST)   -name "*.c" )

#OBJECTS
CC_OBJS=$(patsubst %.cpp,./$(OutPut)/%.o,$(CC_SRC))
CC_OBJS2=$(patsubst %.c,./$(OutPut)/%.o,$(CC_SRC2))
OBJS=$(CC_OBJS)
OBJS2=$(CC_OBJS2)
#DEPS
DEPS=$(patsubst %.o,%.d,$(OBJS))

#DIRS
define OBJ_MKDIR
  OBJ_DIRS+=./$(OutPut)/$(1)
endef
CC_DIRS=$(shell find $(DIR_LIST) -type d|sed -e '/.svn/d')
$(foreach dir,$(CC_DIRS),$(eval $(call OBJ_MKDIR,$(dir))))

#DEPS
DEPS=$(patsubst %.o,%.d,$(OBJS))

#INCLUDE DIR
define SAFE_MKDIR
  INC_DIR+=-I $(1)
endef
$(foreach dir,$(CC_DIRS),$(eval $(call SAFE_MKDIR,$(dir))))

INC_DIR+=-I $(BOOST_ROOT)  

#LIB_DIR
LIB_DIR=-L /usr/local/lib -L $(BOOST_LIB) -L  $(COMLIB)/$(OS)_Gcc$(GCC)  

LIBS=-lsos -lpthread  -lcrypto -lboost_system -ljson
LDFLAGS= $(LIB_DIR) $(LIBS)
CPPFLAGS=$(INC_DIR) $(DFLAGS)  -DTIXML_USE_STL 

EXE1=../../SoPlugin/HpsSignature.so
EXE2=$(EXE1).$(VER)

all:$(EXE1) $(EXE2)
$(shell mkdir -p $(sort $(OBJ_DIRS)))
include $(DEPS)
$(EXE2):$(EXE1)
	if [ -f $@ ]; then rm $@; fi
	ln $^ $@

$(EXE1):$(OBJS) $(OBJS2)
	$(CC) -g -shared -fPIC -o $@ $^ $(LDFLAGS) 

./$(OutPut)/%.o:%.cpp
	$(CC) -g -o $@ -c -fPIC $< $(CPPFLAGS)


./$(OutPut)/%.d:%.cpp
	@set -e; rm -f $@; \
	$(CC) -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,.*\.o[ :]*,$(patsubst %.d,%.o,$@) $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

./$(OutPut)/%.o:%.c
	$(CC) -g -o $@ -c -fPIC $< $(CPPFLAGS)


./$(OutPut)/%.d:%.c
	@set -e; rm -f $@; \
	$(CC) -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,.*\.o[ :]*,$(patsubst %.d,%.o,$@) $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$
test:
	cat $(GCC)
clean:
	rm -rf $(OutPut)
	rm -rf $(EXE1)
codelen:
	find $(NEW_CODE_PATH) \( -name "*.cpp" -o -name "*.h" -o -name "*.c" \) -exec cat {} \;|sed -e 's/\"/\n\"\n/g;s/\([^\/]\)\(\/\*\)/\1\n\2\n/g;'|sed  -e '/^\"/{:a;N;/\".*\"/!ba;s/\".*\".*//;N;/\"/!d;b}' -e '/^\/\*/{s/\/\*.*\*\///;/\/\*/{:b;N;/\/\*.*\*\//!bb;s/\/\*.*\*\///}}' -e 's/\/\/.*//g' |sed -e '/^[[:space:]]*$$/d'|wc -l
srczip:
	zip -r ./$(EXE1).zip * -x *.o *.d *.svn *.zip *.a *.so $(EXE1) *.svn-work *.svn-base *.so.* *.d.* *.svn/*
binzip:
	if [ ! -d "release" ];  then mkdir release; fi;
	cd ./release; if [ ! -d "lib" ];  then mkdir lib; fi;
	cp -r ./BGWAdapter ./release/BGWAdapter
	cp -r ./config.xml ./release
	cp -r ./log.properties ./release
	cp -r ./service ./release
	#cp -r ./Aas_release_note.doc ./release
	ldd ./BGWAdapter>sys_so_111_232_876_23;cp `awk  '{if(substr($$3,1,4)!="/lib"&&substr($$3,1,8)!="/usr/lib")print $$3}' sys_so_111_232_876_23` ./release/lib/;rm -rf sys_so_111_232_876_23
	cp /lib/libcrypto.so.4  ./release/lib;
	cd release; zip -r ../$(VER)_$(OS)_Gcc$(GCC)_X86.zip *
	rm -rf release



