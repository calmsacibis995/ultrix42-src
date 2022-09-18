#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/var/yp $(DESTROOT)/etc

AOUT=	mknetid

OBJS=	mknetid.o getname.o

install:
	$(INSTALL) -s -c mknetid $(DESTROOT)/usr/var/yp/mknetid

mknetid.o:	mknetid.c
getname.o:	getname.c

include $(GMAKERULES)
