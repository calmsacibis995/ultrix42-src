#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	cpio

OBJS=	cpio.o

cpio:	cpio.o
cpio.o:	cpio.c

install:
	$(INSTALL) -c -s cpio $(DESTROOT)/usr/bin/cpio

include $(GMAKERULES)
