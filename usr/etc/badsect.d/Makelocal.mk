#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	badsect

OBJS=	badsect.o

badsect.o:	badsect.c

install:
	$(INSTALL) -c -s badsect $(DESTROOT)/usr/etc/badsect
	$(RM) $(DESTROOT)/etc/badsect
	$(LN) -s ../usr/etc/badsect $(DESTROOT)/etc/badsect

include $(GMAKERULES)
