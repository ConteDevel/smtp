DIST_PATH  = ./dist
BUILD_PATH = ./build

MKDIR_P = mkdir -p

.PHONY: all

ASAN    = -fsanitize=address

SHELL   = /bin/sh
CC      = gcc
AR      = ar
FLAGS   = -std=gnu99
CFLAGS  = -fPIC -pedantic -Wall -Werror -g -ggdb $(ASAN)
LDFLAGS = $(ASAN) -lrt -lresolv

COMMON_HEADERS = -Icommon/include
COMMON_SOURCES = common/log/log.c common/sl.c common/map.c #$(shell find common -type f -name "*.c")
COMMON_OBJECTS = $(addprefix $(BUILD_PATH)/, $(COMMON_SOURCES:.c=.o))
COMMON_TARGET  = $(DIST_PATH)/libcommon.a

CLIENT_HEADERS = -Iclient/include $(COMMON_HEADERS)
CLIENT_SOURCES = $(shell find client -type f -name "*.c")
CLIENT_OBJECTS = $(addprefix $(BUILD_PATH)/, $(CLIENT_SOURCES:.c=.o))
CLIENT_TARGET  = $(DIST_PATH)/client

all: build_common

build_common: dir_common $(COMMON_TARGET)

dir_common:
	$(MKDIR_P) $(DIST_PATH)
	$(MKDIR_P) $(BUILD_PATH)/common
	$(MKDIR_P) $(BUILD_PATH)/common/log

$(BUILD_PATH)/common/%.o: common/%.c
	$(CC) $(FLAGS) $(CFLAGS) $(COMMON_HEADERS) -c $< -o $@

$(BUILD_PATH)/common/log/%.o: common/log/%.c
	$(CC) $(FLAGS) $(CFLAGS) $(COMMON_HEADERS) -c $< -o $@


$(COMMON_TARGET): $(COMMON_OBJECTS)
	$(AR) rcs $@ $^


build_client: build_common dir_client $(CLIENT_TARGET)

dir_client:
	$(MKDIR_P) $(DIST_PATH)
	$(MKDIR_P) $(BUILD_PATH)/client

$(BUILD_PATH)/client/%.o: client/%.c
	$(CC) $(FLAGS) $(CFLAGS) $(CLIENT_HEADERS) -c $< -o $@

$(CLIENT_TARGET): $(CLIENT_OBJECTS)
	$(CC) $(FLAGS) $(CFLAGS) -o $@ $^ $(COMMON_TARGET) $(LDFLAGS)


clear:
	rm -rf build
	rm -rf dist