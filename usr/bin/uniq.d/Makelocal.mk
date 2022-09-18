#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	uniq

OBJS=	uniq.o

uniq:	uniq.o
uniq.o:	uniq.c

install:
	$(INSTALL) -c -s uniq $(DESTROOT)/usr/bin/uniq

include $(GMAKERULES)
