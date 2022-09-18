#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc

AOUT=	nfsportmon

OBJS=	nfsportmon.o

nfsportmon:	nfsportmon.o
nfsportmon.o:	nfsportmon.c

install:
	$(INSTALL) -c -s nfsportmon $(DESTROOT)/usr/etc/nfsportmon

include $(GMAKERULES)
