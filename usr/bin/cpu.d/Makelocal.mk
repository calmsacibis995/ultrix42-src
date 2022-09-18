# @(#)Makelocal.mk	4.1	(ULTRIX)	7/17/90

include $(GMAKEVARS)

LOADLIBES=-lcurses -ltermcap

DESTLIST=$(DESTROOT)/etc \
	 $(DESTROOT)/usr/bin

all:	cpustat startcpu stopcpu

cpustat: cpustat.o
	$(CC) -o cpustat cpustat.o $(LOADLIBES)

startcpu: startcpu.o
	$(CC) -o startcpu startcpu.o

stopcpu: stopcpu.o
	$(CC) -o stopcpu stopcpu.o

cpustat.o:	cpustat.c
startcpu.o:	startcpu.c
stopcpu.o:	stopcpu.c

install:

	$(INSTALL) -c -s -g kmem -m 2711 cpustat $(DESTROOT)/usr/bin/cpustat
	$(INSTALL) -c -s startcpu $(DESTROOT)/etc/startcpu
	$(INSTALL) -c -s stopcpu $(DESTROOT)/etc/stopcpu

include $(GMAKERULES)

