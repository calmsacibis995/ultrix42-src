#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

CINCLUDES=-I../../include -I.. -I$(SRCROOT)/usr/include
CDEFINES=-DDBM  -DLOG -DVMUNIX
LOADLIBES=-ldbm

OBJSMISC=logger.o mconnect.o syslog.o vacation.o

include ../Makelocal_$(MACHINE).mk

all: logger mconnect syslog vacation

logger.o:	logger.c
mconnect.o:	mconnect.c
syslog.o:	syslog.c
vacation.o:	vacation.c

logger: logger.o
	$(CC) -o $@ $@.o $(LOADLIBES)

mconnect: mconnect.o
	$(CC) -o $@ $@.o

syslog: syslog.o
	$(CC) -o $@ $@.o

vacation: vacation.o
	$(CC) -o $@ $@.o ../../src/_$(MACHINE).b/convtime.o $(LOADLIBES)

include $(GMAKERULES)
