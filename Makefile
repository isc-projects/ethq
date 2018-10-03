CXXFLAGS	= -O3 -std=c++11 -Wall -Werror

LDFLAGS		=

LIBS_CURSES	= -lncurses

TARGETS		= ethq

COMMON_OBJS	= timer.o util.o
DRIVER_OBJS	= drv_intel.o drv_vmxnet3.o

all:		$(TARGETS)

ethq:		ethq.o ethtool++.o parser.o util.o $(DRIVER_OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(LIBS_CURSES)

clean:
	$(RM) $(TARGETS) *.o

ethq.o:		ethtool++.h util.h parser.h
parser.o:	parser.h
ethtool++.o:	ethtool++.h util.h
util.o:		util.h
$(DRIVER_OBJS):	parser.h
