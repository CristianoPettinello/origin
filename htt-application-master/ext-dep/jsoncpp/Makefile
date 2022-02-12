#
#          NIDEK TECHNOLOGIES S.R.L. PROPRIETARY INFORMATION
#
#  This software is supplied under the terms of a license agreement or
#  nondisclosure agreement with  Nidek Technologies S.R.L. and may not
#  be copied or disclosed  except in accordance with the terms of that
#  agreement.
#  Copyright(c) 2016   Nidek Technologies S.R.L.   All Rights Reserved.
#

MKDIR=mkdir
DIR=build
MV=mv
CP=rsync -cpot

ROOTPATH=.
TESTSPATH=
LIBPATH=$(ROOTPATH)/src/lib_json

BUILD_ARTIFACTS+=include/json/*.h

NAME=jsoncpp
LIBNAME=lib$(NAME)
OBJFILES=$(LIBPATH)/json_reader.o $(LIBPATH)/json_value.o $(LIBPATH)/json_writer.o
INCLUDES+=-I./include -I$(LIBPATH)
CPP_COMPILE_OPTIONS=-std=c++11 -fPIC
COMPILE_OPTIONS=-W -Wall -c

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
	$(VERBOSE_CMD) $(CP) $(wildcard $(BUILD_ARTIFACTS)) $(DIR)/include/json
	@ $(RM) $(LIBNAME).a

$(LIBNAME).a: $(OBJFILES)
	$(AR) crs $@ $^

%.o: %.cpp $(BUILDDEPS)
	$(VERBOSE_PRE)
	$(VERBOSE_CMD) $(CC) $(COMPILE_OPTIONS) $(CPP_COMPILE_OPTIONS) $(INCLUDES) $< -o $@

clean: $(CLEANDEPS)
	$(VERBOSE_PRE)
	$(VERBOSE_CMD) rm -rf $(DIR) $(LIBPATH)/*.o *.a
