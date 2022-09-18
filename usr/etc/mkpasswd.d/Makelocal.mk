#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/etc $(DESTROOT)/usr/etc

AOUT=	mkpasswd

OBJS=	mkpasswd.o

mkpasswd:	mkpasswd.o
mkpasswd.o:	mkpasswd.c

install:
	$(INSTALL) -c -s -m 751 mkpasswd $(DESTROOT)/usr/etc/mkpasswd
	$(RM) $(DESTROOT)/etc/mkpasswd
	$(LN) -s ../usr/etc/mkpasswd $(DESTROOT)/etc/mkpasswd

include $(GMAKERULES)
