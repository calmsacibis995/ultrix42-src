#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc $(DESTROOT)/etc

AOUT=	trpt

OBJS=	trpt.o

trpt.o:	trpt.c

install:
	$(INSTALL) -c -s trpt $(DESTROOT)/usr/etc/trpt
	$(RM) $(DESTROOT)/etc/trpt
	$(LN) -s ../usr/etc/trpt $(DESTROOT)/etc/trpt

include $(GMAKERULES)
