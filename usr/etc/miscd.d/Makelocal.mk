#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	miscd

OBJS=	miscd.o

miscd.o:	miscd.c

install:
	$(INSTALL) -c -s miscd $(DESTROOT)/usr/etc/miscd
	$(RM) $(DESTROOT)/etc/miscd
	$(LN) -s ../usr/etc/miscd $(DESTROOT)/etc/miscd

include $(GMAKERULES)
