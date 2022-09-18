#  @(#)Makelocal.mk	4.4  ULTRIX  12/6/90

include $(GMAKEVARS)

CFLAGS=-O -DAUTHEN

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

all:	rshd

OBJS=	rshd.o

rshd:	rshd.o
	$(LDCMD) rshd.o -lckrb -lkrb -ldes -lknet -lauth

rshd.o:	rshd.c

install:
	$(INSTALL) -c -s rshd $(DESTROOT)/usr/etc/rshd
	$(RM) $(DESTROOT)/etc/rshd
	$(LN) -s ../usr/etc/rshd $(DESTROOT)/etc/rshd

include $(GMAKERULES)
