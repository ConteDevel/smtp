DIST_PATH = ../dist
LIB_PATH = $(DIST_PATH)/libs
BUILD_PATH = ../build/server

MKDIR_P = mkdir -p
RM_RF = rm -rf

.PHONY: all

SHELL	= /bin/sh
CC		= gcc
FLAGS	= -std=gnu99
CFLAGS	= -fPIC -pedantic -Wall -Werror
LDFLAGS	= -shared

HEADERS = -Iinclude -I../common/include

all: subsystem dirs server

subsystem:
	$(MAKE) -C ../common

dirs : 
	$(MKDIR_P) $(BUILD_PATH)
	
$(BUILD_PATH)/%.o: %.c
	$(CC) $(FLAGS) $(CFLAGS) $(HEADERS) -c $< -o $@

server : $(BUILD_PATH)/main.o
	$(CC) -L$(LIB_PATH) -Wl,-rpath=$(LIB_PATH) $(FLAGS) $(CFLAGS) -o $(DIST_PATH)/server $^ -llog -lrt
	
clean:
	$(MAKE) -C ../common clean
	$(RM_RF) $(DIST_PATH)/server
	$(RM_RF) $(BUILD_PATH)
