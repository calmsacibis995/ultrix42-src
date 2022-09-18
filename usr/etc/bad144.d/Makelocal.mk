#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	bad144

OBJS=	bad144.o

bad144.o:	bad144.c

install:
	$(INSTALL) -c -s bad144 $(DESTROOT)/usr/etc/bad144
	$(RM) $(DESTROOT)/etc/bad144
	$(LN) -s ../usr/etc/bad144 $(DESTROOT)/etc/bad144

include $(GMAKERULES)
