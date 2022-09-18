#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	sa

OBJS=	sa.o

sa.o:	sa.c

install:
	$(INSTALL) -c -s sa $(DESTROOT)/usr/etc/sa
	$(RM) $(DESTROOT)/etc/sa
	$(LN) -s ../usr/etc/sa $(DESTROOT)/etc/sa

include $(GMAKERULES)
