#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	shexp

OBJS=	shexp.o

CFLAGS=-O -DAUTHEN

KRBLIBS=        -lckrb -lkrb -lknet -ldes

AUTHLIB=        -lauth

LOADLIBES = $(KRBLIBS) $(AUTHLIB)

shexp:	shexp.o
shexp.o:	shexp.c

install:
	$(INSTALL) -c -s -o root -g authread -m 6751 shexp $(DESTROOT)/usr/bin/shexp
	$(RM) $(DESTROOT)/bin/shexp
	$(LN) -s ../usr/bin/shexp $(DESTROOT)/bin/shexp

include $(GMAKERULES)
