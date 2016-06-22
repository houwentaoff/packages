################################################################################
################################################################################
##
## Makefile -- makefile for  test tool lib application
##
## (C) Copyright 2012-2014 by Goke Microelectronics Co.,Ltd
##
## Version: $Id: Makefile 2554 2014-12-23 12:35:06Z houwentao $
##
##
##
################################################################################
################################################################################

default: all

INC:= -I../include -I../legacy/include 
LD:=

ifneq ($(PC), 1)
	TOP_DIR = $(shell pwd)/../../
include ../env.mk
endif

CFLAGS+=-g
CFLAGS+=$(INC)
LDFLAGS+=$(LD)

.PHONY: clean

all:lib
	@echo "    compile datatx..."

lib:datatx_lib.o
	$(AR) rucs  libdatatx.a $^

%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	$(RM) *.a *.o



