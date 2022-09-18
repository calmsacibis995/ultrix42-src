#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	hostid

OBJS=	hostid.o

hostid:	hostid.o
hostid.o:	hostid.c

install:
	$(INSTALL) -c -s hostid $(DESTROOT)/usr/bin/hostid
	$(RM) $(DESTROOT)/bin/hostid
	$(LN) -s ../usr/bin/hostid $(DESTROOT)/bin/hostid

include $(GMAKERULES)
