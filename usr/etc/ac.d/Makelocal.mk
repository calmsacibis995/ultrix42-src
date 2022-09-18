#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc $(DESTROOT)/etc

AOUT=	ac

OBJS=	ac.o

ac.o:	ac.c

install:
	$(INSTALL) -c -s ac $(DESTROOT)/usr/etc/ac
	$(RM) $(DESTROOT)/etc/ac
	$(LN) -s ../usr/etc/ac $(DESTROOT)/etc/ac

include $(GMAKERULES)
