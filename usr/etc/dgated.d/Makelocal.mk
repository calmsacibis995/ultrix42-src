#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	dgated

OBJS=	dgated.o

dgated.o:	dgated.c

install:
	$(INSTALL) -c -s dgated $(DESTROOT)/usr/etc/dgated
	$(RM) $(DESTROOT)/etc/dgated
	$(LN) -s ../usr/etc/dgated $(DESTROOT)/etc/dgated

include $(GMAKERULES)
