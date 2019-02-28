DIST_PATH = ../dist
LIB_PATH = $(DIST_PATH)/libs
BUILD_PATH = ../build/server
DEF_PATH = def
GEN_PATH = $(BUILD_PATH)/gen

MKDIR_P = mkdir -p
RM_RF = rm -rf
AUTOGEN = autogen
CP = cp
MV = mv -f

.PHONY: all

SHELL	= /bin/sh
CC		= gcc
FLAGS	= -std=gnu99
GEN_CFLAGS	= -g -DTEST_SERVER `autoopts-config cflags`
GEN_LDFLAGS	= `autoopts-config ldflags`
CFLAGS	= -fPIC -pedantic -Wall -Werror $(GEN_CFLAGS)
LDFLAGS	= -llog -lrt -lconfig $(GEN_LDFLAGS)

HEADERS = -Iinclude -I../common/include -I$(GEN_PATH)
OBJS = $(addprefix $(BUILD_PATH)/, $(patsubst %.c, %.o, $(wildcard *.c))) $(BUILD_PATH)/checkopt.o

all: subsystem dirs gen server conf

subsystem:
	$(MAKE) -C ../common

dirs: 
	$(MKDIR_P) $(GEN_PATH)

gen:
	$(AUTOGEN) $(DEF_PATH)/checkopt.def
	$(MV) checkopt.c checkopt.h $(GEN_PATH)

$(BUILD_PATH)/checkopt.o:
	$(CC) $(FLAGS) $(CFLAGS) $(HEADERS) -c $(GEN_PATH)/checkopt.c -o $(BUILD_PATH)/checkopt.o
 	
$(BUILD_PATH)/%.o: %.c
	$(CC) $(FLAGS) $(CFLAGS) $(HEADERS) -c $< -o $@

server: $(OBJS)
	$(CC) -L$(LIB_PATH) -Wl,-rpath=$(LIB_PATH) $(FLAGS) $(CFLAGS) -o $(DIST_PATH)/server $^ $(LDFLAGS)

conf:
	$(CP) def/*.conf $(DIST_PATH)
	
clean:
	$(MAKE) -C ../common clean
	$(RM_RF) $(DIST_PATH)/server
	$(RM_RF) $(DIST_PATH)/default_server.conf
	$(RM_RF) $(BUILD_PATH)
