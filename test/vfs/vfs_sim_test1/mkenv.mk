

TOP = ../..

CROSS = 
CC = $(CROSS)gcc
CXX = $(CROSS)g++
LD = $(CROSS)ld
AS = $(CC)
AR = $(CROSS)ar
OBJCOPY = $(CROSS)objcopy
OBJDUMP = $(CROSS)objdump

MKDIR = mkdir
CP = cp

BUILD = build
TARGET = $(BUILD)/vfs_simfs_test1

ifeq ($(DEBUG), 1)
CFLAGS += -O0 -g
CXXFLAGS += -O0 -g
ASFLAGS += $(CFLAGS)

else
CFLAGS += -O3
CXXFLAGS += -O3
ASFLAGS += $(CFLAGS)
endif

CFLAGS +=

LDFLAGS +=

INC += -I. -I"$(TOP)" -I"$(TOP)/include"

LIB +=
