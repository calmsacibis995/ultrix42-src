# @(#)Makelocal.mk	4.1	(ULTRIX)	7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ 

AOUT=	kgconv

OBJS=	kgconv.o

kgconv.o:	kgconv.c

install:
	$(INSTALL) -c -s kgconv ${DESTROOT}/usr/etc/kgconv

include $(GMAKERULES)
