SHELL = /bin/bash

PIXELMAN_INSTALL:=/Applications/Pixelman
# if not defined in the env take PWD
ifndef PIXELMAN_INSTALL
	PIXELMAN_INSTALL:=$(PWD)
endif
PIXELMAN_HWLIBS:=$(PIXELMAN_INSTALL)/hwlibs

ROOTCONFIG = $(shell which root-config)
ROOTCINT   = $(shell which rootcint)

#ROOTCFLAGS   := $(shell $(ROOTCONFIG) --prefix=$(ROOTSYS) --cflags)
#ROOTLIBS     := $(shell $(ROOTCONFIG) --prefix=$(ROOTSYS) --libs)
#ROOTGLIBS    := $(shell $(ROOTCONFIG) --prefix=$(ROOTSYS) --glibs)
CXXFLAGS     += $(ROOTCFLAGS) -I./

all: batchpixel #batchpixel_ramp_k2410 k2410_CommandLineControl
	@echo done

frameScan: frameScan.o
	g++ frameScan.o $(ROOTLIBS) $(ROOTGLIBS) -o frameScan

frameScan.o: frameScan.cpp
	echo $(CXXFLAGS)
	g++ -Wall -g3 -c frameScan.cpp $(CXXFLAGS)

batchpixel: batchpixel.o initbatch.o
	g++ -g3 -m64 batchpixel.o initbatch.o -L$(PIXELMAN_INSTALL) -lmpxlicmgr -lmpxmanager -lmpxctrl -L$(PIXELMAN_HWLIBS) -lmpxusb  -ldl -o batchpixel.exe
	cp batchpixel.exe $(PIXELMAN_INSTALL)/

batchpixel.o: batchpixel.cpp 
	g++ -g3 -m64 -Wall  -c -I$(PIXELMAN_INSTALL)/_other_/headers batchpixel.cpp

initbatch.o: initbatch.cpp
	g++ -g3 -m64 -Wall -c -I$(PIXELMAN_INSTALL)/_other_/headers initbatch.cpp

batchpixel_ramp_k2410: batchpixel_ramp_k2410.o
	g++ batchpixel_ramp_k2410.o -lrt -L $(PIXELMAN_INSTALL) -lMpxManager -lMpxCtrl -L$(PIXELMAN_HWLIBS) -lMpxHwUSB2 -lMpxHwUSB -ldl -o batchpixel_ramp_k2410.exe
	cp batchpixel_ramp_k2410.exe ../

batchpixel_ramp_k2410.o:
	g++ -Wall -g3 -c -I$(PIXELMAN_INSTALL)/_other_/headers batchpixel_ramp_k2410.cpp

k2410_CommandLineControl: k2410_CommandLineControl.o
	g++ k2410_CommandLineControl.o -ldl -o k2410_CommandLineControl

k2410_CommandLineControl.o: k2410_CommandLineControl.cpp
	g++ -Wall -g3 -c  k2410_CommandLineControl.cpp

clean:
	rm -f initbatch.o batchpixel.o batchpixel_ramp_k2410.o batchpixel_ramp_k2410.exe batchpixel.exe
