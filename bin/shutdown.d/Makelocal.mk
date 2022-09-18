#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin $(DESTROOT)/etc

LOADLIBES=-lerrlog

AOUT=	shutdown

OBJS=	shutdown.o

shutdown.o:	shutdown.c

install:
	$(INSTALL) -c -s shutdown $(DESTROOT)/bin/shutdown
	$(RM) $(DESTROOT)/etc/shutdown
	$(LN) -s ../bin/shutdown $(DESTROOT)/etc/shutdown

include $(GMAKERULES)
