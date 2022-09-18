#  @(#)Makelocal.mk	4.1	ULTRIX	10/15/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/

AOUT=	traceroute

OBJS=	traceroute.o

traceroute.o:	traceroute.c

install:
	$(INSTALL) -c -s traceroute $(DESTROOT)/usr/etc/traceroute

include $(GMAKERULES)
