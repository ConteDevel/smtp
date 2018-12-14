DIST_PATH = ../dist
LIB_PATH = $(DIST_PATH)/libs
BUILD_PATH = ../build

MKDIR_P = mkdir -p

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
	$(MKDIR_P) $(BUILD_PATH)/server
	
$(BUILD_PATH)/server/%.o: %.c
	$(CC) $(FLAGS) $(CFLAGS) $(HEADERS) -c $< -o $@

server : $(BUILD_PATH)/server/main.o
	$(CC) -L$(LIB_PATH) -Wl,-rpath=$(LIB_PATH) $(FLAGS) $(CFLAGS) -o $(DIST_PATH)/server $^ -llog -lrt
