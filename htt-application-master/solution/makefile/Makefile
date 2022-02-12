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

ROOTPATH=../..
TESTSPATH=$(ROOTPATH)/tests/ImageProcessing.Tester
LIBPATH=$(ROOTPATH)/libraries/ImageProcessing

EXT-DEPPATH=$(ROOTPATH)/ext-dep
EXTLIBPATH=-L$(EXT-DEPPATH)/jsoncpp/build
EXTLIBS=-ljsoncpp

UTILSPATH=$(ROOTPATH)/libraries/Utils
UTILSLIBPATH=-L$(UTILSPATH)/build
UTILSLIBS=-lutils

INCLUDES+=-I./include -I$(LIBPATH)/include -I$(LIBPATH)/src -I$(EXT-DEPPATH)/jsoncpp/build/include -I$(UTILSPATH)/build/include
LIBRARIES=-Wl,--start-group $(EXTLIBS) $(UTILSLIBS) -Wl,--end-group -lpng -llog4cpp -lm -lstdc++ -lpthread

CPP_COMPILE_OPTIONS=-std=c++11 -fPIC
COMPILE_OPTIONS=$(DEFINES) -W -Wall -c -O3 #-g #-pg
LINK_OPTIONS= #-pg

NAME=main
OBJFILES=$(LIBPATH)/src/ImagePreprocessing.o $(LIBPATH)/src/Profile.o \
$(LIBPATH)/src/MiniBevelModel.o $(LIBPATH)/src/MiniBevelModelExt.o \
$(LIBPATH)/src/ScheimpflugTransform.o $(LIBPATH)/src/Timer.o \
$(LIBPATH)/src/OptimizationEngine.o $(LIBPATH)/src/Model.o $(LIBPATH)/src/Pso.o \
$(LIBPATH)/src/HttPrivate.o $(LIBPATH)/src/Statistics.o $(LIBPATH)/src/Tester.o \
$(LIBPATH)/src/TBevelModel.o $(LIBPATH)/src/CustomBevelModel.o $(LIBPATH)/src/Hough.o $(LIBPATH)/src/Htt.o

ifeq ($(VERBOSE),yes)
	VERBOSE_PRE=@ echo "[$(NAME)] $@"
	VERBOSE_CMD=
else
	VERBOSE_PRE=
	VERBOSE_CMD=@ echo "[$(NAME)] $@" &
endif

default: $(NAME) test

$(NAME): $(TESTSPATH)/src/$(NAME).o $(OBJFILES)
	@ $(MKDIR) -p $(DIR)/conf
	$(VERBOSE_CMD) $(CC) -o $@ $^ $(LINK_OPTIONS) $(EXTLIBPATH) $(UTILSLIBPATH) $(LIBRARIES)
	$(MV) $(NAME) $(DIR) && $(CP) $(TESTSPATH)/src/settings.json $(TESTSPATH)/src/calibration.json $(TESTSPATH)/src/log4cpp.properties $(DIR)/conf

test: $(TESTSPATH)/src/test.o $(OBJFILES)
	$(VERBOSE_CMD) $(CC) -o $@ $^ $(LINK_OPTIONS) $(EXTLIBPATH) $(UTILSLIBPATH) $(LIBRARIES)
	$(MV) test $(DIR)

%.o: %.cpp $(BUILDDEPS)
	$(VERBOSE_PRE)
	$(VERBOSE_CMD) $(CC) $(COMPILE_OPTIONS) $(CPP_COMPILE_OPTIONS) $(INCLUDES)  $< -o $@

clean: $(CLEANDEPS)
	$(VERBOSE_PRE)
	$(VERBOSE_CMD) rm -rf $(DIR) $(OBJFILES) $(TESTSPATH)/src/*.o
