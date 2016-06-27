OS=$(shell uname)$(shell uname -a | sed -n '1p' | perl -nle 'print $$1 if /\s+([0-9]\.\d+)/')
GCC=$(shell gcc --version | sed -n '1p' | perl -nle 'print $$1 if /\s+([0-9]\.\d+)/')
CC=g++
DIR_LIST=./coh ./sap ./src  ../../release
OutPut=build/self/self

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

INC_DIR+=-I /home/zongjinliang/local/src/boost_1_59_0
#LIB_DIR
LIB_DIR=-L/usr/local/lib -L /home/zongjinliang/local/src/boost_1_59_0/stage/lib -L ../../release/$(OS)_Gcc$(GCC)
LIBS=-lboost_unit_test_framework -Wl,-Bstatic -lsap -lcoh -lcommon -lxlog -ltinyxml -lboost_date_time -lboost_thread -lboost_system -Wl,-Bdynamic  -lpthread  -lcrypto  -lrt

LDFLAGS= $(LIB_DIR) $(LIBS)
CPPFLAGS=$(INC_DIR) $(DFLAGS)  -DTIXML_USE_STL -Winvalid-pch -Wall


EXE1=./test

all:$(EXE1)
$(shell mkdir -p $(sort $(OBJ_DIRS)))
include $(DEPS)

$(EXE1):$(OBJS) $(OBJS2)
	$(CC) -g -o $@ $^ $(LDFLAGS)
	
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
