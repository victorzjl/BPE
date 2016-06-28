OS=$(shell uname)$(shell uname -a | sed -n '1p' | perl -nle 'print $$1 if /\s+([0-9]\.\d+)/')
GCC=$(shell gcc --version | sed -n '1p' | perl -nle 'print $$1 if /\s+([0-9]\.\d+)/')
VER_PT=$(shell bit=`getconf LONG_BIT`;if [ $$bit -eq 64 ];  then echo 'X86-64'; else echo 'X86'; fi;)LIB_PT=$(shell bit=`getconf LONG_BIT`;if [ $$bit -eq 64 ];  then echo '_X86-64'; else echo ''; fi;)

OS=$(shell uname)$(shell uname -a | sed -n '1p' | perl -nle 'print $$1 if /\s+([0-9]\.\d+)/')
GCC=$(shell gcc --version | sed -n '1p' | perl -nle 'print $$1 if /\s+([0-9]\.\d+)/')
CC=g++
VER=1.0.1.3
COMLIB=/usr/local/avenue/stack/release
SOS_COMLIB=/usr/local/avenue/sos
DIR_LIST=./src ./httpClient

OutPut=build/
NEW_CODE_PATH=./
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

#OBJ_DIRS+=./$(OutPut)/$(COMLIB)/include ./$(OutPut)/asynclog ./$(OutPut)/business ./$(OutPut)/coh ./$(OutPut)/httpAgent ./$(OutPut)/interface ./$(OutPut)/selfcheck ./$(OutPut)/src ./$(OutPut)/transfer
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


INC_DIR+=-I $(BOOST_ROOT) -I $(COMLIB)/include/common -I $(COMLIB)/include/xlog -I $(COMLIB)/include/ -I $(COMLIB)/include/cohstack -Wall

#LIB_DIR

LIB_DIR=-L/usr/local/lib -L./osap -L $(BOOST_LIB) -L $(COMLIB)/$(OS)_Gcc$(GCC) -L $(SOS_COMLIB)/release/$(OS)_Gcc$(GCC)$(LIB_PT)

#LIBS= -lsos -lboost_serialization -lmemcached   -lpthread  -lcrypto -lclntsh -Wall  
LIBS=-lcommon -lcoh -lxlog -ltinyxml -lboost_serialization -lpthread  -lcrypto -lboost_date_time -lboost_thread -lboost_system  -Wall -ldl -Wl,--export-dynamic
#LIBS=-lSDOOsapSDK -lCommonSDK -lcommon -lcoh -lxlog -ltinyxml -lboost_serialization -lpthread  -lcrypto -lboost_date_time -lboost_thread -lboost_system  -Wall -ldl -Wl,--export-dynamic

LDFLAGS= $(LIB_DIR) $(LIBS)
CPPFLAGS=$(INC_DIR) $(DFLAGS)  -DTIXML_USE_STL 

EXE1=./test

all:$(EXE1)
$(shell mkdir -p $(sort $(OBJ_DIRS)))
include $(DEPS)

$(EXE1):$(OBJS) $(OBJS2)
	$(CC) -g  -O2 -o $@ $^ $(LDFLAGS)
	ar -r ./build/libhps.a $^
#	ar -r ./build/libhttpAgent.a ./build/httpAgent/*.o
#	ar -r ./build/libasynclog.a ./build/asynclog/*.o
#	ar -r ./build/libbusiness.a ./build/business/*.o
#	ar -r ./build/libcoh.a ./build/coh/*.o
#	ar -r ./build/libsapstack.a ./build/sapstack/*.o
#	ar -r ./build/libtransfer.a ./build/transfer/*.o
	
./$(OutPut)/%.o:%.cpp
	$(CC) -g -O2 -o $@ -c -fPIC $< $(CPPFLAGS)


./$(OutPut)/%.d:%.cpp
	@set -e; rm -f $@; \
	$(CC) -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,.*\.o[ :]*,$(patsubst %.d,%.o,$@) $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

./$(OutPut)/%.o:%.c
	$(CC) -g  -O2 -o $@ -c -fPIC $< $(CPPFLAGS)


./$(OutPut)/%.d:%.c
	@set -e; rm -f $@; \
	$(CC) -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,.*\.o[ :]*,$(patsubst %.d,%.o,$@) $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

clean:
	rm -Rf $(OutPut)
	rm -rf $(EXE1)
codelen:
	find $(NEW_CODE_PATH) \( -name "*.cpp" -o -name "*.h" -o -name "*.c" \) -exec cat {} \;|sed -e 's/\"/\n\"\n/g;s/\([^\/]\)\(\/\*\)/\1\n\2\n/g;'|sed  -e '/^\"/{:a;N;/\".*\"/!ba;s/\".*\".*//;N;/\"/!d;b}' -e '/^\/\*/{s/\/\*.*\*\///;/\/\*/{:b;N;/\/\*.*\*\//!bb;s/\/\*.*\*\///}}' -e 's/\/\/.*//g' |sed -e '/^[[:space:]]*$$/d'|wc -l
srczip:
	zip -r ./hps_src_$(VER).zip * -x *.o *.d *.svn *.zip *.a *.so $(EXE1) *.svn-work *.svn-base *.so.* *.d.* *.svn/*
binzip:
	if [ ! -d "release" ];  then mkdir release; fi;
	cd release;if [ ! -d "lib" ];  then mkdir lib; fi;  
	cd release;if [ ! -d "plugin" ];  then mkdir plugin; fi;  
	cp -r ./hps ./release
	cp -r ./config.xml.release ./release/config.xml
	cp -r ./service ./release
	cp -r ./log.properties.release ./release/log.properties
	#cp -r ./plugin ./release
	cp ./osap/*.so ./release/lib
	cp ./osap/client.conf ./release
	cp ./osap/Setting.xml ./release
	cp ./osap/KeyWordId.xml ./release
	cp -r ./osap/services ./release
	cp /lib/libcrypto.so.4 ./release/lib
	cp /usr/local/lib/libjson.so.0.0.1 ./release/lib/libjson.so.0
	
	ldd ./hps>sys_so_111_232_876_23;cp `awk  '{if(substr($$3,1,4)!="/lib"&&substr($$3,1,8)!="/usr/lib")print $$3}' sys_so_111_232_876_23` ./release/lib/;rm -rf sys_so_111_232_876_23
	cd release; zip -r ../HPS_$(VER)_$(OS)_Gcc$(GCC)_X86.zip *
	rm -rf release


