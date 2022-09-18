#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc $(DESTROOT)/etc

AOUT=	nfsstat

OBJS=	nfsstat.o

nfsstat.o:	nfsstat.c

install:
	$(INSTALL) -c -m 2711 -g kmem -s nfsstat $(DESTROOT)/usr/etc/nfsstat
	$(RM) $(DESTROOT)/etc/nfsstat
	$(LN) -s ../usr/etc/nfsstat $(DESTROOT)/etc/nfsstat

include $(GMAKERULES)
