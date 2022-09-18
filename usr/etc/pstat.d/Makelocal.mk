#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc $(DESTROOT)/etc

AOUT=	pstat

OBJS=	pstat.o

pstat:		pstat.o
pstat.o:	pstat.c


install:
	$(INSTALL) -c -m 2711 -g kmem -s pstat $(DESTROOT)/usr/etc/pstat
	$(RM) $(DESTROOT)/etc/pstat
	$(LN) -s ../usr/etc/pstat $(DESTROOT)/etc/pstat

include $(GMAKERULES)
