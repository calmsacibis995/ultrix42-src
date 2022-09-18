#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc

CFLAGS=-O -DAUTHEN

KRBLIBS=        -lckrb -lkrb -lknet -ldes

AUTHLIB=        -lauth

LOADLIBES = $(KRBLIBS) $(AUTHLIB)

AOUT=ftpd

OBJS=ftpd.o glob.o vers.o ftpcmd.o

ftpd.o:ftpd.c
glob.o:glob.c

ftpcmd.o:ftpcmd.y

vers.o:	vers.c
	$(CCCMD) vers.c
vers.c:
	$(SHELL) ../newvers.sh

install:
	$(INSTALL) -c -s ftpd $(DESTROOT)/usr/etc/ftpd

include $(GMAKERULES)
