#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	dcheck

OBJS=	dcheck.o

dcheck.o:	dcheck.c

install:
	$(INSTALL) -c -s dcheck $(DESTROOT)/etc/dcheck

include $(GMAKERULES)
