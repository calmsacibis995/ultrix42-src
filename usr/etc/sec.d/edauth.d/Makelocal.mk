#  @(#)Makelocal.mk	4.2  ULTRIX  8/13/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/sec

SYDIR=../../../../../sys/sys
AOUTS=	edauth

LOADLIBES=	-lauth -laud

edauth:	edauth.o syscalls.o

edauth.o:	edauth.c

syscalls.o:	$(SYDIR)/syscalls.c 
	$(CCCMD) $(SYDIR)/syscalls.c

install:
	$(INSTALL) -c -s -m 700 edauth $(DESTROOT)/usr/etc/sec/edauth

include $(GMAKERULES)
