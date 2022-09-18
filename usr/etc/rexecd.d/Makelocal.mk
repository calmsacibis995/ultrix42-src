#  @(#)Makelocal.mk	4.2  ULTRIX  12/6/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	rexecd
OBJS=	rexecd.o

CDEFINES=	-DAUTHEN

KRBLIBS=	-lckrb -lkrb -lknet -ldes
AUTHLIB=	-lauth
LOADLIBES=	$(KRBLIBS) $(AUTHLIB)

rexecd.o:	rexecd.c

install:
	$(INSTALL) -c -s rexecd $(DESTROOT)/usr/etc/rexecd
	$(RM) $(DESTROOT)/etc/rexecd
	$(LN) -s ../usr/etc/rexecd $(DESTROOT)/etc/rexecd

include $(GMAKERULES)
