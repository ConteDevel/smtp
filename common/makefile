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

LOG_SRCS = log/log.c
LOG_HEADERS = -Iinclude/log
LOG_OBJS = $(addprefix $(BUILD_PATH)/log/, $(notdir $(LOG_SRCS:.c=.o)))
LOG_TARGET = $(LIB_PATH)/liblog.so

all: dirs $(LOG_TARGET)

dirs : 
	$(MKDIR_P) $(LIB_PATH)
	$(MKDIR_P) $(BUILD_PATH)/log
	
$(BUILD_PATH)/log/%.o: log/%.c
	$(CC) $(FLAGS) $(CFLAGS) $(LOG_HEADERS) -c $< -o $@

$(LOG_TARGET) : $(LOG_OBJS)
	$(CC) $(FLAGS) $(CFLAGS) -o $@ $^ $(LDFLAGS)

samples : $(BUILD_PATH)/log/main.o
	$(CC) -L$(LIB_PATH) -Wl,-rpath=$(LIB_PATH) $(FLAGS) $(CFLAGS) -o $(DIST_PATH)/log_sample $^ -llog 
