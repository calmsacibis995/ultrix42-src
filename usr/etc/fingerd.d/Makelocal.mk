#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	fingerd

OBJS=	fingerd.o

fingerd.o:	fingerd.c

install:
	$(INSTALL) -c -s fingerd $(DESTROOT)/usr/etc/fingerd
	$(RM) $(DESTROOT)/etc/fingerd
	$(LN) -s ../usr/etc/fingerd $(DESTROOT)/etc/fingerd

include $(GMAKERULES)
