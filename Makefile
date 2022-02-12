#
#          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
#
#  This software is supplied under the terms of a license agreement or
#  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
#  be copied or disclosed  except in accordance with the terms of that
#  agreement.
#  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
#

ifneq ($(OS),Windows_NT)
DEFINES+=-DUNIX
endif

MKDIR=mkdir
DIR=build
MV=mv
CP=rsync -cpot

ROOTPATH=.
BUILD_ARTIFACTS=$(ROOTPATH)/include/*.h

EXT-DEPPATH=../../ext-dep
EXTLIBPATH=-L$(EXT-DEPPATH)/jsoncpp/build
# EXTLIBS=-ljsoncpp

NAME=utils
LIBNAME=lib$(NAME)
OBJFILES=./src/Log.o ./src/Image.o ./src/Settings.o ./src/Utils.o
INCLUDES=-I./include -I$(EXT-DEPPATH)/jsoncpp/build/include
CPP_COMPILE_OPTIONS=-std=c++11 -fPIC
COMPILE_OPTIONS=$(DEFINES) -W -Wall -c -g

ifeq ($(VERBOSE),yes)
	VERBOSE_PRE=@ echo "[$(NAME)] $@"
	VERBOSE_CMD=
else
	VERBOSE_PRE=
	VERBOSE_CMD=@ echo "[$(NAME)] $@" &
endif

default: lib

.PHONY : all clean

lib: $(LIBNAME).a
	@ $(MKDIR) -p $(DIR)/include
	$(VERBOSE_CMD) $(CP) $(LIBNAME).a  $(DIR)/$(LIBNAME).a
	$(VERBOSE_CMD) $(CP) $(wildcard $(BUILD_ARTIFACTS)) $(DIR)/include
	@ $(RM) $(LIBNAME).a

$(LIBNAME).a: $(OBJFILES)
	$(AR) crs $@ $^

%.o: %.cpp $(BUILDDEPS)
	$(VERBOSE_PRE)
	$(VERBOSE_CMD) $(CC) $(COMPILE_OPTIONS) $(CPP_COMPILE_OPTIONS) $(INCLUDES) $< -o $@ 

clean: $(CLEANDEPS)
	$(VERBOSE_PRE)
	$(VERBOSE_CMD) rm -rf $(DIR) $(OBJFILES) *.a
