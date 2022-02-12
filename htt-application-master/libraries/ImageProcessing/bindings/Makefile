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

ROOTPATH=../../..
BUILDDIR=$(ROOTPATH)/solution/makefile/build
INCLUDES+=-I../include -I../../../ext-dep/jsoncpp/include
COMPILE_OPTIONS=$(DEFINES) -O3 -Wall -shared -std=c++11 -fPIC `python3 -m pybind11 --includes`
LIBS=-ljsoncpp
LIBPATH=-L $(ROOTPATH)/ext-dep/jsoncpp/build
OBJFILES=$(BUILDDIR)/Htt.o $(BUILDDIR)/Settings.o $(BUILDDIR)/Log.o $(BUILDDIR)/Image.o $(BUILDDIR)/ImagePreprocessing.o $(BUILDDIR)/Profile.o $(BUILDDIR)/Utils.o
ifneq ($(OS),Windows_NT)
OBJFILES+=$(BUILDDIR)/lodepng.o
LIBS+=-llog4cpp -lpng
endif

default: all

all:
ifneq ($(OS),Windows_NT)
	$(CC) $(COMPILE_OPTIONS) $(INCLUDES) pyHtt.cpp -o pyHtt`python3-config --extension-suffix` $(OBJFILES) $(LIBPATH) $(LIBS)
else
	$(CC) $(COMPILE_OPTIONS) $(INCLUDES) pyHtt.cpp -o pyHtt.so $(OBJFILES) $(OBJFILES2) $(LIBPATH) $(LIBS) -L$(ROOTPATH)/solution/makefile -lhtt
endif
clean:
	rm -rf *.so
