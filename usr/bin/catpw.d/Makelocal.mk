# @(#)Makelocal.mk	4.1 ULTRIX 7/17/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/bin

AOUT=catpw

OBJS=catpw.o

catpw.o: catpw.c

install:
	$(INSTALL) -c -s catpw $(DESTROOT)/usr/bin/catpw

include $(GMAKERULES)
