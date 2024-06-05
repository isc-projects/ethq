CXXFLAGS	= -O3 -std=c++11 -Wall -Werror -ffat-lto-objects

LDFLAGS		= -s

LIBS_CURSES	= -lncurses -ltinfo

TARGETS		= ethq ethq_test

DRIVER_OBJS	= drv_generic.o \
		  drv_bcm.o drv_emulex.o drv_intel.o drv_mellanox.o \
		  drv_amazon.o drv_virtio.o drv_vmware.o drv_sfc.o \
		  drv_nxp.o

all:		$(TARGETS)

ethq:		ethq.o ethtool++.o interface.o parser.o util.o $(DRIVER_OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(LIBS_CURSES)

ethq_test:	ethq_test.o parser.o util.o $(DRIVER_OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

clean:
	$(RM) $(TARGETS) *.o

ethq.o:		interface.h util.h
ethq_test.o:	parser.h util.h
parser.o:	parser.h
ethtool++.o:	ethtool++.h util.h
interface.o:	interface.h
interface.h:	parser.h optval.h optval.h
util.o:		util.h
$(DRIVER_OBJS):	parser.h
