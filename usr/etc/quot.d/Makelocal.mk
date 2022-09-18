#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	quot

OBJS=	quot.o

quot.o:	quot.c

install:
	$(INSTALL) -c -s quot $(DESTROOT)/usr/etc/quot
	$(RM) $(DESTROOT)/etc/quot
	$(LN) -s ../usr/etc/quot $(DESTROOT)/etc/quot

include $(GMAKERULES)
