OBJDIR := .
CXXFLAGS := -Wall -g
TARGETDIR := .

#TARGETDIR := $(HOME)/projects/work/targets
#include ../common/Makefile.inc

CXXFLAGS := $(CXXFLAGS) -I ../common/ -I/usr/include/ -I/usr/include/ncurses -g 

OBJECTS := P8Mutex.o P8Socket.o P8EthernetSocket.o P8PrologixWrangler.o P8InstrumentWrangler.o Thread.o P8SlowLogger.o NcursesInterface.o
TARGETS := prologix_test p8_logger visual_daq
SCRIPTS := sensors.config

vpath %.o $(OBJDIR)

LIBS :=
LINKEDLIBS := -lm -lpthread -lncurses

OBJECTS_WDIR = $(patsubst %.o,$(OBJDIR)/%.o,$(OBJECTS))
TARGETS_WDIR = $(patsubst %,$(TARGETDIR)/%,$(TARGETS))
SCRIPTS_WDIR = $(patsubst %,$(TARGETDIR)/%,$(SCRIPTS))
TARGETOBJECTS_WDIR = $(patsubst %,$(OBJDIR)/%.o,$(TARGETS))
LIBS_WDIR = $(patsubst %,$(OBJDIR)/%,$(LIBS))

all: $(OBJECTS_WDIR) $(TARGETS_WDIR) $(SCRIPTS_WDIR)

$(OBJECTS_WDIR) : $(OBJDIR)/%.o : %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGETOBJECTS_WDIR) : $(OBJDIR)/%.o : %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGETS_WDIR) : $(TARGETDIR)/% : $(OBJDIR)/%.o $(OBJECTS_WDIR)
	$(CXX) $(CXXFLAGS) $< $(OBJECTS_WDIR) $(LIBS_WDIR) $(LINKEDLIBS) -o $@

$(SCRIPTS_WDIR) : $(TARGETDIR)/% : %
	cp $< $@


clean:
	rm -f $(OBJECTS_WDIR)
	rm -f $(TARGETOBJECTS_WDIR)
	rm -f $(TARGETS_WDIR)
