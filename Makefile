CXXFLAGS	= -O3 -std=c++11 -Wall -Werror

LDFLAGS		=

LIBS_CURSES	= -lncurses

TARGETS		= ethq

COMMON_OBJS	= timer.o util.o

all:		$(TARGETS)

ethq:		ethq.o ethtool++.o util.o
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(LIBS_CURSES)

clean:
	$(RM) $(TARGETS) *.o

ethtool++.o:	ethtool++.h

util.o:		util.h
