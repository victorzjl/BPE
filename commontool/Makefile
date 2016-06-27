#OS=`uname``uname -a | sed -n '1p' | perl -nle 'print $$1 if /\s+([0-9]\.\d+)/'`
#GCC=`gcc --version | sed -n '1p' | perl -nle 'print $$1 if /\s+([0-9]\.\d+)/'`
OS=$(shell uname)$(shell uname -a | sed -n '1p' | perl -nle 'print $$1 if /\s+([0-9]\.\d+)/')
GCC=$(shell gcc --version | sed -n '1p' | perl -nle 'print $$1 if /\s+([0-9]\.\d+)/')
VER_PT=$(shell bit=`getconf LONG_BIT`;if [ $$bit -eq 64 ];  then echo 'X86-64'; else echo 'X86'; fi;)
LIB_PT=$(shell bit=`getconf LONG_BIT`;if [ $$bit -eq 64 ];  then echo '_X86-64'; else echo ''; fi;)

COMLIB=/usr/local/avenue/stack/release/
CC=g++ 
VER=1.1.0
DIR_LIST=./src  ./test
OutPut=build/self/
NEW_CODE_PATH=./sap
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
        #@echo $(CC_DIRS)
$(foreach dir,$(CC_DIRS),$(eval $(call OBJ_MKDIR,$(dir))))

#DEPS
DEPS=$(patsubst %.o,%.d,$(OBJS))

#INCLUDE DIR
define SAFE_MKDIR
  INC_DIR+=-I $(1)
endef
$(foreach dir,$(CC_DIRS),$(eval $(call SAFE_MKDIR,$(dir))))
INC_DIR+=-I $(BOOST_ROOT)
INC_DIR+=-I $(COMLIB)/include/common/ -I $(COMLIB)/include/sapstack/ -I $(COMLIB)/include/ -I $(COMLIB)/include/xlog/


#LIB_DIR
LIB_DIR=-L $(BOOST_LIB) -L $(COMLIB)/$(OS)_Gcc$(GCC)
LIBS=-Wl,-Bstatic -lcommon -lxlog -ltinyxml -lboost_regex -lboost_date_time -lboost_thread -lboost_system -Wl,-Bdynamic -lpthread -lcrypto

LDFLAGS= $(LIB_DIR) $(LIBS)
CPPFLAGS=$(INC_DIR) $(DFLAGS)  -DTIXML_USE_STL

EXE1=./build/libCommonTools.so
EXE2=./build/libCommonTools.a

all:$(EXE1) $(EXE2)
$(shell mkdir -p $(sort $(OBJ_DIRS)))
include $(DEPS)

$(EXE1):$(OBJS) $(OBJS2)
	mkdir -p ./stacklib
	cd ./stacklib;cp -r $(COMLIB)/$(OS)_Gcc$(GCC)/libxlog.a ./;ar -x libxlog.a
#	$(CC)  -fPIC -shared -o $@ $^ ./stacklib/*.o $(LDFLAGS)
	rm -fr ./stacklib
$(EXE2):$(OBJS)  $(OBJS2)
	mkdir -p ./stacklib
	cd ./stacklib;cp -r $(COMLIB)/$(OS)_Gcc$(GCC)/libsap.a ./;ar -x libsap.a
	cd ./stacklib;cp -r $(COMLIB)/$(OS)_Gcc$(GCC)/libxlog.a ./;ar -x libxlog.a
	cd ./stacklib;cp -r $(COMLIB)/$(OS)_Gcc$(GCC)/libcommon.a ./;ar -x libcommon.a
	cd ./stacklib;cp -r $(COMLIB)/$(OS)_Gcc$(GCC)/libtinyxml.a ./;ar -x libtinyxml.a
	cd ./stacklib;cp -r $(BOOST_LIB)/libboost_date_time.a ./;ar -x libboost_date_time.a
	cd ./stacklib;cp -r $(BOOST_LIB)/libboost_thread.a ./;ar -x libboost_thread.a
	cd ./stacklib;cp -r $(BOOST_LIB)/libboost_system.a ./;ar -x libboost_system.a
	cd ./stacklib;cp -r $(BOOST_LIB)/libboost_regex.a ./;ar -x libboost_regex.a
	cd ./stacklib;cp -r /usr/lib/libcrypto.a ./;ar -x libcrypto.a
	ar -r $@ $^ ./stacklib/*.o
	ar -r $@ $^
	rm -fr ./stacklib	

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
test:$(EXE1)
	g++ ./test/test.cpp -o ./commontool -L ./build -lCommonTools   -lpthread $(CPPFLAGS) 
clean:
	rm -Rf $(OutPut)
	rm -rf $(EXE1)
	rm -rf $(EXE2)
binzip:
	mkdir -p release
	mkdir -p release/include
	mkdir -p release/services/
	mkdir -p release/$(OS)_Gcc$(GCC)$(LIB_PT)
	cp -r ./build/*.a ./release/$(OS)_Gcc$(GCC)$(LIB_PT)/
	cp -r ./build/*.so ./release/$(OS)_Gcc$(GCC)$(LIB_PT)/
	cp -r ./CommonToolsHandler.h ./release/include
	cp -r ../conf/log.properties ./release/
	cp -r ../conf/services/*.xml ./release/services/
	cd release; zip -r ../CommonTools_$(OS)_Gcc$(GCC)_X86.zip *


