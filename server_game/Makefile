OSFLAG 				:=
ifeq ($(OS),Windows_NT)
	OSFLAG += -D WIN32
	ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
		OSFLAG += -D AMD64
	endif
	ifeq ($(PROCESSOR_ARCHITECTURE),x86)
		OSFLAG += -D IA32
	endif
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		OSFLAG += -D LINUX
	endif
	ifeq ($(UNAME_S),Darwin)
		OSFLAG += -D OSX
	endif
	UNAME_P := $(shell uname -p)
	ifeq ($(UNAME_P),x86_64)
		OSFLAG += -D AMD64
	endif
		ifneq ($(filter %86,$(UNAME_P)),)
	OSFLAG += -D IA32
		endif
	ifneq ($(filter arm%,$(UNAME_P)),)
		OSFLAG += -D ARM
	endif
endif

LIBS := -luv
INC := -I. -I../xxlib -I../libuv/include -I../sqlite3 -I../chipmunk/include/chipmunk \
	-I../gens/output

CFLAGS += -std=c++17 -g

ifeq ($(findstring OSX, $(OSFLAG)),)
	CC = g++
	LIBS += ../chipmunk/lib/lin64/libchipmunk.a
else
	CC = llvm-g++
	LIBS += ../chipmunk/lib/mac/libchipmunk.a
	INC += -I/usr/local/opt/mysql@5.7/include
	CFLAGS += -D__OSX__=1
endif


spaths = ./ ../xxlib ../public ../pkgclass ../chipmunk/include/chipmunk

prevFiles = Makefile

BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
DEST_DIR = dest
BIN_DIR = $(DEST_DIR)/bin
D_DIR = $(BUILD_DIR)/d

vpath %o $(OBJ_DIR)
vpath %d $(D_DIR)
vpath %dpp $(D_DIR)
vpath %h $(spaths)
vpath %cpp $(spaths)
vpath %hpp $(spaths)
vpath %c $(spaths)

# cs := ikcp.c http_parser.c
# files := PKG_class.hpp CatchFish.hpp main.cpp
cs := ikcp.c
files := main.cpp
hpps = $(filter %.hpp,$(files))
cpps = $(filter %.cpp,$(files))

obj_c = $(cs:.c=.o)
obj_cpp = $(cpps:.cpp=.o)
obj_cpp += $(hpps:.hpp=.o)

obj = $(obj_cpp) $(obj_c)

d_cpps = $(cpps:.cpp=.dpp)
d_cpps += $(hpps:.hpp=.dpp)
d_c = $(cs:.c=.d)

define gen_d
@set -e; rm -f $@; \
$(CC) -M $(INC) $(CFLAGS) $< > $(D_DIR)/$@.$$$$; \
sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $(D_DIR)/$@.$$$$ > $(D_DIR)/$@; \
rm -f $(D_DIR)/$@.$$$$
endef

server: makeDirs $(prevFiles) $(obj)
	$(CC) -Wall $(INC) $(CFLAGS) -o $(BIN_DIR)/game.out \
	$(OBJ_DIR)/ikcp.o \
	$(OBJ_DIR)/main.o $(LIBS)
	-cp -rf ../gens/output/cfg.bin $(DEST_DIR)

include $(wildcard $(D_DIR)/*.dpp)
include $(wildcard $(D_DIR)/*.d)

%.d: %.c
	$(gen_d)

%.dpp: %.cpp
	$(gen_d)

ikcp.o: ikcp.d ikcp.c $(prevFiles)
	$(CC) -Wall $(INC) $(CFLAGS) -c ../xxlib/ikcp.c -o $(OBJ_DIR)/ikcp.o

main.o: main.cpp main.dpp $(prevFiles)
	$(CC) -Wall $(INC) $(CFLAGS) -c main.cpp -o $(OBJ_DIR)/main.o

.PHONY: clean makeDirs

makeDirs:
	-mkdir -p $(OBJ_DIR) $(D_DIR) $(DEST_DIR) $(BIN_DIR)
	# -echo $(obj_cpp)

clean:
	-rm -rf $(OBJ_DIR) $(D_DIR)
