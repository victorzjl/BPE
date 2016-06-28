OS=$(shell uname)$(shell uname -a | sed -n '1p' | perl -nle 'print $$1 if /\s+([0-9]\.\d+)/')
GCC=$(shell gcc --version | sed -n '1p' | perl -nle 'print $$1 if /\s+([0-9]\.\d+)/')
VER_PT=$(shell bit=`getconf LONG_BIT`;if [ $$bit -eq 64 ];  then echo 'X86-64'; else echo 'X86'; fi;)LIB_PT=$(shell bit=`getconf LONG_BIT`;if [ $$bit -eq 64 ];  then echo '_X86-64'; else echo ''; fi;)
STACK_COMLIB=/usr/local/avenue/stack
CC=g++
VER=1.0.2.17
DIR_LIST=./common ./bussiness ./include ./thirdparty ./db  ./tinysap  ./dbconn ./dbconfig ./asynlog
OutPut=build/self/
NEW_CODE_PATH=./common ./bussiness ./include ./thirdparty ./db ./dbconn ./dbconfig ./tinysap ./asynlog
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


INC_DIR+=-I $(BOOST_ROOT) -I $(STACK_COMLIB)/release/include/stack/   -I /usr/local/include/mysql/ -I $(STACK_COMLIB)/release/include/xlog -I $(STACK_COMLIB)/release/include/sapstack -I $(STACK_COMLIB)/release/include/ -I $(ORACLE_HOME)/rdbms/public -I/home/zhaogang/unixodbc/include

#LIB_DIR
LIB_DIR=-L./lib -L/usr/local/lib -L $(BOOST_LIB) -L $(STACK_COMLIB)/release/$(OS)_Gcc$(GCC)$(LIB_PT) -L$(ORACLE_HOME)/lib   -L./lib/x/

LIBS=  -lboost_system -lcommon -lxlog -lsap -ltinyxml -lcrypto -lclntsh -lmysqlclient -lsoci_oracle -lsoci_core -lsoci_mysql -lsoci_odbc -lodbc -lpthread -lboost_thread -lboost_filesystem

LDFLAGS= $(LIB_DIR) $(LIBS)
CPPFLAGS=$(INC_DIR) $(DFLAGS)  -DTIXML_USE_STL -Wall  

EXE1=./libdbp.so

all:$(EXE1)
$(shell mkdir -p $(sort $(OBJ_DIRS)))
include $(DEPS)

$(EXE1):$(OBJS) $(OBJS2)
	mkdir -p ./stacklib
	cd ./stacklib;cp -r $(STACK_COMLIB)/release/$(OS)_Gcc$(GCC)$(LIB_PT)/libsap.a ./;ar -x libsap.a
	cd ./stacklib;cp -r $(STACK_COMLIB)/release/$(OS)_Gcc$(GCC)$(LIB_PT)/libxlog.a ./;ar -x libxlog.a
	cd ./stacklib;cp -r $(BOOST_LIB)/libboost_system.a ./;ar -x libboost_system.a
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
	rm -rf build
codelen:
	find $(NEW_CODE_PATH) \( -name "*.cpp" -o -name "*.h" -o -name "*.c" \) -exec cat {} \;|sed -e 's/\"/\n\"\n/g;s/\([^\/]\)\(\/\*\)/\1\n\2\n/g;'|sed  -e '/^\"/{:a;N;/\".*\"/!ba;s/\".*\".*//;N;/\"/!d;b}' -e '/^\/\*/{s/\/\*.*\*\///;/\/\*/{:b;N;/\/\*.*\*\//!bb;s/\/\*.*\*\///}}' -e 's/\/\/.*//g' |sed -e '/^[[:space:]]*$$/d'|wc -l
srczip:
	zip -r ./$(EXE1).zip * -x *.o *.d *.svn *.zip *.a *.so $(EXE1) *.svn-work *.svn-base *.so.* *.d.* *.svn/*
	
binzip:
	if [ ! -d "release" ];  then mkdir release; fi;
	cd release;if [ ! -d ""async_virtual_service ];  then mkdir async_virtual_service; fi;
	cd release;if [ ! -d ""avenue_conf ]; then mkdir avenue_conf; fi;
	cd release;if [ ! -d ""compose_conf ]; then mkdir compose_conf; fi;
	cd release;if [ ! -d ""lib ]; then mkdir lib; fi;
	cp -r $(EXE1) ./release/virtual_service/
	cp ./config/avenue_conf/* ./release/avenue_conf/ -rfv
	cp ./config/compose_conf/* ./release/compose_conf/ -rfv
	cp ./config/config.xml ./release/ -rfv
	cp ./lib/x/*.so.* ./release/lib/ -rfv
	cp ./lib/x/*.so   ./release/lib/ -rfv
	cd release; zip -r ../LIBDBP_$(VER)_$(OS)_Gcc$(GCC)_X86.zip *
	rm -rf release
