#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	swapon

OBJS=	swapon.o

swapon.o:	swapon.c

install:
	$(INSTALL) -c -s swapon $(DESTROOT)/usr/etc/swapon
	$(RM) $(DESTROOT)/etc/swapon
	$(LN) -s ../usr/etc/swapon $(DESTROOT)/etc/swapon

include $(GMAKERULES)
