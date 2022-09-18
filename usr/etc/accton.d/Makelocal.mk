#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	accton

OBJS=	accton.o

accton.o:	accton.c

install:
	$(INSTALL) -c -s accton $(DESTROOT)/usr/etc/accton
	$(RM) $(DESTROOT)/etc/accton
	$(LN) -s ../usr/etc/accton $(DESTROOT)/etc/accton

include $(GMAKERULES)
