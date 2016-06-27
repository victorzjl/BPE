OS=$(shell uname)$(shell uname -a | sed -n '1p' | perl -nle 'print $$1 if /\s+([0-9]\.\d+)/')
GCC=$(shell gcc --version | sed -n '1p' | perl -nle 'print $$1 if /\s+([0-9]\.\d+)/')
VER_PT=$(shell bit=`getconf LONG_BIT`;if [ $$bit -eq 64 ];  then echo 'X86-64'; else echo 'X86'; fi;)LIB_PT=$(shell bit=`getconf LONG_BIT`;if [ $$bit -eq 64 ];  then echo '_X86-64'; else echo ''; fi;)
STACK_COMLIB=/usr/local/avenue/stack
SOS_COMLIB=/usr/local/avenue/sos
CC=g++
ASC_VER=2.3.4.0
BSC_VER=1.0.0.7
DIR_LIST= ./business ./asynclog ./coh ./tools
OutPut=build/self/
NEW_CODE_PATH=./business  ./src   ./asynclog ./tools
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
OBJ_DIRS+=build/self/src

#DEPS
DEPS=$(patsubst %.o,%.d,$(OBJS))

#INCLUDE DIR
define SAFE_MKDIR
  INC_DIR+=-I $(1)
endef
$(foreach dir,$(CC_DIRS),$(eval $(call SAFE_MKDIR,$(dir))))

INC_DIR+=-I $(BOOST_ROOT) -I $(STACK_COMLIB)/release/include/common -I lua/include -I $(STACK_COMLIB)/release/include/xlog -I $(STACK_COMLIB)/release/include/ -I $(STACK_COMLIB)/release/include/cohstack 
#LIB_DIR
LIB_DIR=-L/usr/local/lib -L $(BOOST_LIB) -L lua/lib -L $(STACK_COMLIB)/release/$(OS)_Gcc$(GCC)$(LIB_PT) -L $(SOS_COMLIB)/release/$(OS)_Gcc$(GCC)$(LIB_PT)
#LIBS= -lboost_date_time -lboost_thread -lboost_system -lboost_regex -lpthread -lcommon -lcoh -lxlog -ltinyxml -lcrypto -luuid -ldl -Wl,--export-dynamic
LIBS= -lboost_date_time -lboost_thread -lboost_system -lboost_regex -lpthread -ltcmalloc_minimal -lcommon -lcoh -lxlog -ltinyxml -lcrypto -luuid -llua -ldl -Wl,--export-dynamic 
LDFLAGS= $(LIB_DIR) $(LIBS)
CPPFLAGS=$(INC_DIR) $(DFLAGS)  -DTIXML_USE_STL  -Wall  

EXE1=./bpe
EXE2=./bsc

all:$(EXE1) $(EXE2)
$(shell mkdir -p $(sort $(OBJ_DIRS))|rm -rf tools/CommonToolsHandler.*)
include $(DEPS)


$(EXE1):$(OBJS) $(OBJS2)
	$(CC) -g  -O3 -o build/self//./src/asc.o -c -fPIC  ./src/asc.cpp $(CPPFLAGS)
	$(CC) -g  -O3 -o $@  build/self//./src/asc.o $^ $(LDFLAGS)
$(EXE2):$(OBJS) $(OBJS2)
	$(CC) -g  -O3 -o build/self//./src/bsc.o -c -fPIC ./src/bsc.cpp $(CPPFLAGS)
	$(CC) -g  -O3 -o $@  build/self//./src/bsc.o $^ $(LDFLAGS)
	
./$(OutPut)/%.o:%.cpp
	$(CC) -g  -O3 -o $@ -c -fPIC $< $(CPPFLAGS)


./$(OutPut)/%.d:%.cpp
	@set -e; rm -f $@; \
	$(CC) -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,.*\.o[ :]*,$(patsubst %.d,%.o,$@) $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

./$(OutPut)/%.o:%.c
	$(CC) -g -O3 -o $@ -c -fPIC $< $(CPPFLAGS)


./$(OutPut)/%.d:%.c
	@set -e; rm -f $@; \
	$(CC) -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,.*\.o[ :]*,$(patsubst %.d,%.o,$@) $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

clean:
	rm -rf $(OutPut)
	rm -rf $(EXE1)
codelen:
	find $(NEW_CODE_PATH) \( -name "*.cpp" -o -name "*.h" -o -name "*.c" \) -exec cat {} \;|sed -e 's/\"/\n\"\n/g;s/\([^\/]\)\(\/\*\)/\1\n\2\n/g;'|sed  -e '/^\"/{:a;N;/\".*\"/!ba;s/\".*\".*//;N;/\"/!d;b}' -e '/^\/\*/{s/\/\*.*\*\///;/\/\*/{:b;N;/\/\*.*\*\//!bb;s/\/\*.*\*\///}}' -e 's/\/\/.*//g' |sed -e '/^[[:space:]]*$$/d'|wc -l
srczip:
	zip -r ./$(EXE1).zip * -x *.o *.d *.svn *.zip *.a *.so $(EXE1) *.svn-work *.svn-base *.so.* *.d.* *.svn/*
binzip:
	if [ ! -d "release" ];  then mkdir release; fi;
	cd release;if [ ! -d "lib" ];  then mkdir lib; fi;
	cd release;if [ ! -d "avenue_conf" ]; then mkdir avenue_conf; fi;
	cd release;if [ ! -d "compose_conf" ]; then mkdir compose_conf; fi;
	cd release;if [ ! -d "virtual_service" ]; then mkdir virtual_service; fi;
	cd release;if [ ! -d "async_virtual_service" ]; then mkdir async_virtual_service; fi;
	cp -r $(EXE1) ./release
	cp -r ./config.xml.asc ./release/config.xml
	cp -r ./hd_config.xml ./release/hd_config.xml
	cp -r ./trigger_config.xml ./release/trigger_config.xml
	cp -r ./service ./release
	cp -r ./avenue_conf/*.xml ./release/avenue_conf
	cp -r ./virtual_service/* ./release/virtual_service
	cp -r ./async_virtual_service/* ./release/async_virtual_service
	cp -r ./compose_conf/*.xml ./release/compose_conf
	cp -r ./log.properties.release ./release/log.properties
	cp -r ./hd_log.properties ./release/hd_log.properties
	cp -r ./CacheMonitor.xml ./release/CacheMonitor.xml
	cp ./lib/* ./release/lib
	cp /lib/libcrypto.so.4 ./release/lib
	ldd ./bpe>sys_so_111_232_876_23;cp `awk  '{if(substr($$3,1,4)!="/lib"&&substr($$3,1,8)!="/usr/lib")print $$3}' sys_so_111_232_876_23` ./release/lib/;rm -rf sys_so_111_232_876_23
	cd release; zip -r ../BPE_$(ASC_VER)_$(OS)_Gcc$(GCC)_X86.zip *
	rm -rf release
	if [ ! -d "release" ];  then mkdir release; fi;
	cd release;if [ ! -d "lib" ];  then mkdir lib; fi;
	cp -r ./bsc ./release
	cp -r ./config.xml.bsc ./release/config.xml
	cp -r ./service.bsc ./release/service
	cp -r ./log.properties.release ./release/log.properties
	cp /lib/libcrypto.so.4 ./release/lib
	ldd ./bsc>sys_so_111_232_876_23;cp `awk  '{if(substr($$3,1,4)!="/lib"&&substr($$3,1,8)!="/usr/lib")print $$3}' sys_so_111_232_876_23` ./release/lib/;rm -rf sys_so_111_232_876_23
	cd release; zip -r ../BSC_$(BSC_VER)_$(OS)_Gcc$(GCC)_X86.zip *
	rm -rf release

