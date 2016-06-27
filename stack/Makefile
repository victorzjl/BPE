#OS=`uname``uname -a | sed -n '1p' | perl -nle 'print $$1 if /\s+([0-9]\.\d+)/'`
#GCC=`gcc --version | sed -n '1p' | perl -nle 'print $$1 if /\s+([0-9]\.\d+)/'`
OS=$(shell uname)$(shell uname -a | sed -n '1p' | perl -nle 'print $$1 if /\s+([0-9]\.\d+)/')
GCC=$(shell gcc --version | sed -n '1p' | perl -nle 'print $$1 if /\s+([0-9]\.\d+)/')
VER_PT=$(shell bit=`getconf LONG_BIT`;if [ $$bit -eq 64 ];  then echo 'X86-64'; else echo 'X86'; fi;)
LIB_PT=$(shell bit=`getconf LONG_BIT`;if [ $$bit -eq 64 ];  then echo '_X86-64'; else echo ''; fi;)

CC=g++
DIR_LIST=./common ./cohstack  ./comlib ./tinyxml ./sapstack
OutPut=build
NEW_CODE_PATH=./common ./cohstack ./sapstack
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
CC_DIRS=$(shell find $(DIR_LIST) -type d|sed -e '/.svn/d;/comlib\/include/d')
$(foreach dir,$(CC_DIRS),$(eval $(call OBJ_MKDIR,$(dir))))

#DEPS
DEPS=$(patsubst %.o,%.d,$(OBJS))

#INCLUDE DIR
define SAFE_MKDIR
  INC_DIR+=-I $(1)
endef
$(foreach dir,$(CC_DIRS),$(eval $(call SAFE_MKDIR,$(dir))))

INC_DIR+=-I /home/zongjinliang/local/src/boost_1_59_0 -I ./ -I ./comlib/include/xlog 
#LIB_DIR
LIB_DIR=-L/usr/local/lib -L /home/zongjinliang/local/src/boost_1_59_0/stage/lib -L ./comlib/$(OS)_Gcc$(GCC)$(LIB_PT)
LIBS=-lboost_date_time -lboost_thread -lboost_system -lpthread -lxlog -ltinyxml -lcrypto
LDFLAGS= $(LIB_DIR) $(LIBS)
CPPFLAGS=$(INC_DIR) $(DFLAGS)  -DTIXML_USE_STL -Wall

EXE1=./sap

all:$(EXE1)
$(shell mkdir -p $(sort $(OBJ_DIRS)))
include $(DEPS)

$(EXE1):$(OBJS) $(OBJS2)
	ar -r ./build/libcommon.a ./build/common/*.o
	ar -r ./build/libcoh.a ./build/cohstack/*.o 
	ar -r ./build/libsap.a ./build/sapstack/*.o 
	ar -r ./build/libtinyxml.a ./build/tinyxml/*.o 
	$(CC) -fPIC -shared -o ./build/libcommon.so ./build/common/*.o 
	$(CC) -fPIC -shared -o ./build/libcoh.so ./build/cohstack/*.o 
	$(CC) -fPIC -shared -o ./build/libsap.so ./build/sapstack/*.o 
	$(CC) -fPIC -shared -o ./build/libtinyxml.so ./build/tinyxml/*.o 
	
./$(OutPut)/%.o:%.cpp
	$(CC) -g -O3 -o $@ -c -fPIC $< $(CPPFLAGS)


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
codelen:
	find $(NEW_CODE_PATH) \( -name "*.cpp" -o -name "*.h" -o -name "*.c" \) -exec cat {} \;|sed -e 's/\"/\n\"\n/g;s/\([^\/]\)\(\/\*\)/\1\n\2\n/g;'|sed  -e '/^\"/{:a;N;/\".*\"/!ba;s/\".*\".*//;N;/\"/!d;b}' -e '/^\/\*/{s/\/\*.*\*\///;/\/\*/{:b;N;/\/\*.*\*\//!bb;s/\/\*.*\*\///}}' -e 's/\/\/.*//g' |sed -e '/^[[:space:]]*$$/d'|wc -l
srczip:
	zip -r ./$(EXE1).zip * -x *.o *.d *.svn *.zip *.a *.so $(EXE1) *.svn-work *.svn-base *.so.* *.d.* *.svn/*
binzip:
	mkdir -p release
	mkdir -p release/include
	mkdir -p release/include/common
	mkdir -p release/include/common/detail
	mkdir -p release/include/cohstack
	mkdir -p release/include/sapstack
	mkdir -p release/include/tinyxml
	mkdir -p release/$(OS)_Gcc$(GCC)$(LIB_PT)
	mkdir -p release/include/xlog
	cp -r ./build/*.a ./release/$(OS)_Gcc$(GCC)$(LIB_PT)/
	cp -r ./common/*.h ./release/include/common/
	cp -r ./common/detail/*.h ./release/include/common/detail/
	cp -r ./cohstack/*.h ./release/include/cohstack/
	cp -r ./sapstack/*.h ./release/include/sapstack/
	cp -r ./tinyxml/*.h ./release/include/tinyxml/
	cp -r ./comlib/include/xlog/*.h ./release/include/xlog/

