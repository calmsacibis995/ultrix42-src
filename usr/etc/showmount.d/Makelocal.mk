#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc $(DESTROOT)/etc

AOUT=	showmount

OBJS=	showmount.o

showmount.o:	showmount.c

install:
	$(INSTALL) -c -s showmount $(DESTROOT)/usr/etc/showmount
	$(RM) $(DESTROOT)/etc/showmount
	$(LN) -s ../usr/etc/showmount $(DESTROOT)/etc/showmount

include $(GMAKERULES)
