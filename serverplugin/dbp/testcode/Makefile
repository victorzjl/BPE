OS=$(shell uname)$(shell uname -a | sed -n '1p' | perl -nle 'print $$1 if /\s+([0-9]\.\d+)/')
GCC=$(shell gcc --version | sed -n '1p' | perl -nle 'print $$1 if /\s+([0-9]\.\d+)/')
VER_PT=$(shell bit=`getconf LONG_BIT`;if [ $$bit -eq 64 ];  then echo 'X86-64'; else echo 'X86'; fi;)LIB_PT=$(shell bit=`getconf LONG_BIT`;if [ $$bit -eq 64 ];  then echo '_X86-64'; else echo ''; fi;)
STACK_COMLIB=/usr/local/avenue/stack
CC=g++
VER=1.0.0.1
DIR_LIST=  ../include ../thirdparty 
OutPut=build/self/
NEW_CODE_PATH=../include ../thirdparty 
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


INC_DIR+=-I $(BOOST_ROOT) -I $(STACK_COMLIB)/release/include/common  -I /usr/local/include/mysql/ -I $(STACK_COMLIB)/release/include/xlog -I $(STACK_COMLIB)/release/include/sapstack -I $(STACK_COMLIB)/release/include/ -I $(ORACLE_HOME)/rdbms/public -I ../tinysap

#LIB_DIR
LIB_DIR=-L/usr/local/lib -L $(BOOST_LIB) -L $(STACK_COMLIB)/release/$(OS)_Gcc$(GCC)$(LIB_PT)  -L../lib

LIBS= -lboost_system  -lsoci_oracle -lsoci_core

LDFLAGS= $(LIB_DIR) $(LIBS)
CPPFLAGS=$(INC_DIR) $(DFLAGS)  -DTIXML_USE_STL -Wall 

EXE1=./test

all:$(EXE1)
$(shell mkdir -p $(sort $(OBJ_DIRS)))
include $(DEPS)

$(EXE1):test.cpp
	$(CC) -g  -o test test.cpp  $(INC_DIR)  $(LIB_DIR)  -ldl  ../libdbp.so
	

