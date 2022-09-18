#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	mkproto

OBJS=	mkproto.o

mkproto.o:	mkproto.c

install:
	$(INSTALL) -c -s mkproto $(DESTROOT)/usr/etc/mkproto
	$(RM) $(DESTROOT)/etc/mkproto
	$(LN) -s ../usr/etc/mkproto $(DESTROOT)/etc/mkproto

include $(GMAKERULES)
