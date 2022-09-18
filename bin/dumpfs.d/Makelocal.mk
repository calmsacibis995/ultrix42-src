#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin $(DESTROOT)/etc

AOUT=	dumpfs

OBJS=	dumpfs.o

dumpfs.o:	dumpfs.c

install:
	$(INSTALL) -c -s dumpfs $(DESTROOT)/bin/dumpfs
	$(RM) $(DESTROOT)/etc/dumpfs
	$(LN) -s ../bin/dumpfs ${DESTROOT}/etc/dumpfs


include $(GMAKERULES)
