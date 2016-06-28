
OS=$(shell uname)$(shell uname -a | sed -n '1p' | perl -nle 'print $$1 if /\s+([0-9]\.\d+)/')
GCC=$(shell gcc --version | sed -n '1p' | perl -nle 'print $$1 if /\s+([0-9]\.\d+)/')
VER_PT=$(shell bit=`getconf LONG_BIT`;if [ $$bit -eq 64 ];  then echo 'X86-64'; else echo 'X86'; fi;)LIB_PT=$(shell bit=`getconf LONG_BIT`;if [ $$bit -eq 64 ];  then echo '_X86-64'; else echo ''; fi;)
STACK_COMLIB=/usr/local/avenue/stack
#LD_INCLUDE=~/usr/include/
#LD_LIB=~/usr/lib/
CC=g++
VER=1.0.0.1
DIR_LIST= ./src 
OutPut=build/self/
NEW_CODE_PATH=./src  
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

INC_DIR+= -I $(BOOST_ROOT) -I $(STACK_COMLIB)/release/include/common -I $(STACK_COMLIB)/release/include/xlog -I $(STACK_COMLIB)/release/include/sapstack -I $(STACK_COMLIB)/release/include/
#LIB_DIR
LIB_DIR=-L/usr/local/lib -L $(STACK_COMLIB)/release/$(OS)_Gcc$(GCC)$(LIB_PT) -L./
#LIBS= -lxlog -lsap -lz  -lboost_thread  -lpthread -lcommon -lprotocol
LIBS= -lxlog -lsap -lz  -lpthread -lcommon -ltinyxml
LDFLAGS= $(LIB_DIR) $(LIBS)
CPPFLAGS=$(INC_DIR) $(DFLAGS)  -DTIXML_USE_STL -Wall 

EXE1=./libtimer.so
#EXE1=./libvcavs.so

all:$(EXE1)
$(shell mkdir -p $(sort $(OBJ_DIRS)))
include $(DEPS)

$(EXE1):$(OBJS) $(OBJS2)
	mkdir -p ./stacklib
	cd ./stacklib;cp -r $(STACK_COMLIB)/release/$(OS)_Gcc$(GCC)$(LIB_PT)/libsap.a ./;ar -x libsap.a
	cd ./stacklib;cp -r $(STACK_COMLIB)/release/$(OS)_Gcc$(GCC)$(LIB_PT)/libxlog.a ./;ar -x libxlog.a
	cd ./stacklib;cp -r /usr/lib/libboost_thread.a ./;ar -x libboost_thread.a
	$(CC) -g -fPIC -shared -o $@ $^ ./stacklib/*.o $(LDFLAGS) 
	
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

clean:
	rm -rf $(OutPut)
	rm -rf $(EXE1)
	rm -rf stacklib
codelen:
	find $(NEW_CODE_PATH) \( -name "*.cpp" -o -name "*.h" -o -name "*.c" \) -exec cat {} \;|sed -e 's/\"/\n\"\n/g;s/\([^\/]\)\(\/\*\)/\1\n\2\n/g;'|sed  -e '/^\"/{:a;N;/\".*\"/!ba;s/\".*\".*//;N;/\"/!d;b}' -e '/^\/\*/{s/\/\*.*\*\///;/\/\*/{:b;N;/\/\*.*\*\//!bb;s/\/\*.*\*\///}}' -e 's/\/\/.*//g' |sed -e '/^[[:space:]]*$$/d'|wc -l
srczip:
	zip -r ./$(EXE1).zip * -x *.o *.d *.svn *.zip *.a *.so $(EXE1) *.svn-work *.svn-base *.so.* *.d.* *.svn/*
binzip:
	if [ ! -d "release" ];  then mkdir release; fi;
	cd release;if [ ! -d ""virtual_service ];  then mkdir virtual_service; fi;
	cp -r $(EXE1) ./release/virtual_service/
	cd release; zip -r ../LIBLSC_$(VER)_$(OS)_Gcc$(GCC)_X86.zip *
	rm -rf release
