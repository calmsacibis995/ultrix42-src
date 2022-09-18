#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

OBJS=	su.o

AUTHLIB=	-lauth

KRBLIBS=	-lckrb -lkrb -lknet -ldes 

all:	su

su:	su.o
	$(LDCMD) su.o $(KRBLIBS) $(AUTHLIB)

su.o:	su.c
	$(CCCMD) ../su.c
	$(CCCMD) -DAUTHEN ../su.c

install:
	$(INSTALL) -c -s -m 4755 su $(DESTROOT)/usr/bin/su
	$(RM) $(DESTROOT)/bin/su
	$(LN) -s ../usr/bin/su $(DESTROOT)/bin/su

include $(GMAKERULES)
