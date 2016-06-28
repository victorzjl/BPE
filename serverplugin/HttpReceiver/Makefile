OS=$(shell uname)$(shell uname -a | sed -n '1p' | perl -nle 'print $$1 if /\s+([0-9]\.\d+)/')
GCC=$(shell gcc --version | sed -n '1p' | perl -nle 'print $$1 if /\s+([0-9]\.\d+)/')
VER_PT=$(shell bit=`getconf LONG_BIT`;if [ $$bit -eq 64 ];  then echo 'X86-64'; else echo 'X86'; fi;)LIB_PT=$(shell bit=`getconf LONG_BIT`;if [ $$bit -eq 64 ];  then echo '_X86-64'; else echo ''; fi;)

OS=$(shell uname)$(shell uname -a | sed -n '1p' | perl -nle 'print $$1 if /\s+([0-9]\.\d+)/')
GCC=$(shell gcc --version | sed -n '1p' | perl -nle 'print $$1 if /\s+([0-9]\.\d+)/')
CC=g++
VER=1.0.3.32
COMLIB=/usr/local/avenue/stack/release
SOS_COMLIB=/usr/local/avenue/sos
DIR_LIST=./AsyncLog ./Business ./Coh ./HttpAgent ./Interface ./SapStack ./Main ./Transfer ./HttpClient ./Common ./FileManager

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


INC_DIR+=-I $(BOOST_ROOT) -I $(COMLIB)/include/common -I $(COMLIB)/include/xlog -I $(COMLIB)/include/ -I $(COMLIB)/include/cohstack  -I /usr/local/include/json  -Wall

#LIB_DIR

LIB_DIR=-L/usr/local/lib -L./osap -L $(BOOST_LIB) -L $(COMLIB)/$(OS)_Gcc$(GCC) -L $(SOS_COMLIB)/release/$(OS)_Gcc$(GCC)$(LIB_PT)

LIBS=-ltcmalloc_minimal -lmysqlpp -lboost_regex -luuid -lcommon -lcoh -ljson -lxlog -ltinyxml -lboost_serialization -lpthread  -lcrypto -lboost_date_time -lboost_thread -lboost_system -lboost_filesystem -Wall -ldl
#LIBS=-lmysqlpp -lboost_regex -luuid -lcommon -lcoh -ljson -lxlog -ltinyxml -lboost_serialization -lpthread  -lcrypto -lboost_date_time -lboost_thread -lboost_system -lboost_filesystem -Wall -ldl -Wl,--export-dynamic


LDFLAGS= $(LIB_DIR) $(LIBS)
CPPFLAGS=$(INC_DIR) $(DFLAGS) -DSDG_SINGLETON_PLUGIN -DTIXML_USE_STL #-DM_SUPPORT_PUBLIC

EXE1=libhttprecevier.so
EXE2=libhttprecevier.a

all:$(EXE1) $(EXE2)
$(shell mkdir -p $(sort $(OBJ_DIRS)))
include $(DEPS)

$(EXE1):$(OBJS) $(OBJS2)
	$(CC) -Wall -g -O3 -o  $@ $^ $(LDFLAGS) -shared -Wl,-soname,$@
	ar -r $(EXE2) $^
	
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
	rm -rf $(EXE2)
	
codelen:
	find $(NEW_CODE_PATH) \( -name "*.cpp" -o -name "*.h" -o -name "*.c" \) -exec cat {} \;|sed -e 's/\"/\n\"\n/g;s/\([^\/]\)\(\/\*\)/\1\n\2\n/g;'|sed  -e '/^\"/{:a;N;/\".*\"/!ba;s/\".*\".*//;N;/\"/!d;b}' -e '/^\/\*/{s/\/\*.*\*\///;/\/\*/{:b;N;/\/\*.*\*\//!bb;s/\/\*.*\*\///}}' -e 's/\/\/.*//g' |sed -e '/^[[:space:]]*$$/d'|wc -l
srczip:
	zip -r ./hps_src_$(VER).zip * -x *.o *.d *.svn *.zip *.a *.so $(EXE1) *.svn-work *.svn-base *.so.* *.d.* *.svn/*
binzip:
	if [ ! -d "release" ];  then mkdir release; fi;
	cd release;if [ ! -d "lib" ];  then mkdir lib; fi;  
	cd release;if [ ! -d "plugin" ];  then mkdir plugin; fi;
	#cd release;if [ ! -d "osap" ];  then mkdir osap; fi;
	cd release;if [ ! -d "plugin_ReqA2H" ];  then mkdir plugin_ReqA2H; fi;
	cp ./hps ./release
	#cp -r ./config.xml.release ./release/config.xml
	cp -r ./service ./release
	cp ./log.properties ./release/log.properties
	cp ./*.xml ./release/
	cp /lib/libcrypto.so.4 ./release/lib
	cp /lib/libmysqlclient.so.16 ./release/lib/
	cp /usr/local/lib/libmysqlpp.so.3 ./release/lib/libmysqlpp.so.3
	cp /usr/local/lib/libjson.so.0.0.1 ./release/lib/libjson.so.0
	
	ldd ./hps>sys_so_111_232_876_23;cp `awk  '{if(substr($$3,1,4)!="/lib"&&substr($$3,1,8)!="/usr/lib")print $$3}' sys_so_111_232_876_23` ./release/lib/;rm -rf sys_so_111_232_876_23
	cd release; zip -r ../HPS_$(VER)_$(OS)_Gcc$(GCC)_X86.zip *
	rm -rf release


