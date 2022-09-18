#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc $(DESTROOT)/etc

AOUT=	savecore

OBJS=	savecore.o

savecore.o:	savecore.c

install:
	$(INSTALL) -s -c savecore $(DESTROOT)/usr/etc/savecore
	$(RM) $(DESTROOT)/etc/savecore
	$(LN) -s ../usr/etc/savecore $(DESTROOT)/etc/savecore

include $(GMAKERULES)
