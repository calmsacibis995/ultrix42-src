#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	getty

OBJS=	main.o init.o subr.o gettytab.o get_date.o

main.o:		main.c gettytab.h
init.o:		init.c gettytab.h
subr.o:		subr.c gettytab.h
gettytab.o:	gettytab.c
get_date.o:	get_date.c

install:
	$(INSTALL) -c -s getty $(DESTROOT)/usr/etc/getty
	$(RM) $(DESTROOT)/etc/getty
	$(LN) -s ../usr/etc/getty $(DESTROOT)/etc/getty

include $(GMAKERULES)
