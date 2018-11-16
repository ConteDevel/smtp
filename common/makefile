DIST_PATH = ../dist/libs
BUILD_PATH = ../build

MKDIR_P = mkdir -p

.PHONY: all

SHELL   = /bin/sh
CC      = gcc
FLAGS   = -std=gnu99
CFLAGS  = -fPIC -pedantic -Wall -Werror
LDFLAGS = -shared

COMMON_HEADERS = -Iinclude

LOG_SRCS = $(shell echo log/*.c)
LOG_HEADERS = -Iinclude/log
LOG_OBJS = $(addprefix $(BUILD_PATH)/log/, $(notdir $(LOG_SRCS:.c=.o)))
LOG_TARGET = $(DIST_PATH)/liblog.so

all: dirs $(LOG_TARGET)

dirs:
	$(MKDIR_P) $(DIST_PATH)
	$(MKDIR_P) $(BUILD_PATH)/log
    
$(BUILD_PATH)/log/%.o: log/%.c
	$(CC) $(FLAGS) $(CFLAGS) $(COMMON_HEADERS) $(LOG_HEADERS) -c $< -o $@

$(LOG_TARGET) : $(LOG_OBJS)
	$(CC) $(FLAGS) $(CFLAGS) -o $@ $^ $(LDFLAGS)

