#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90
#
#  Lisa Allgood  4th June 1990
#  Fixed release date to 30-APR-1990

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin $(DESTROOT)/usr/bin

KRBLIBS= -lckrb -lkrb -lknet -ldes

all:	login 

login:	login.o
	$(LDCMD) login.o $(KRBLIBS) -lauth -llmf 

login.o:	login.c ultrix_date
	$(CCCMD) -DULTRIX_RELEASE_DATE=641516399 -DAUTHEN ../login.c

ultrix_date: ultrix_date.c
	$(CC) -o $@ ../$?

install:
	$(INSTALL) -c -s -m 4755 login $(DESTROOT)/usr/bin/login
	$(RM) $(DESTROOT)/bin/login
	$(LN) -s ../usr/bin/login $(DESTROOT)/bin/login

include $(GMAKERULES)
