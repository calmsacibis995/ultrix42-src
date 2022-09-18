#  @(#)Makelocal.mk	4.2  ULTRIX  10/12/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin $(DESTROOT)/etc

AOUTS=	init init.sas

OBJS=	init.o init.sas.o

init.o:	init.c
init.sas.o: init.sas.c
init:	init.o
init.sas:	init.sas.o

init.sas.c: init.c
	-rm -f init.sas.c
	cp ../init.c init.sas.c

init.sas.o:
	$(CCCMD) -DSTANDALONE init.sas.c

include ../Makelocal_$(MACHINE).mk

install:
	$(INSTALL) -c -s -o root -g system -m 755 init ${DESTROOT}/bin/init
	$(INSTALL) -c -s -o root -g system -m 755 init.sas ${DESTROOT}/bin/init.sas
	$(RM) $(DESTROOT)/etc/init
	$(LN) -s /bin/init $(DESTROOT)/etc/init

include $(GMAKERULES)
