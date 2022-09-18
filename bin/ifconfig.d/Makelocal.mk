#  @(#)Makelocal.mk	4.2  ULTRIX  10/12/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin $(DESTROOT)/etc

AOUTS=	ifconfig ifconfig.sas

OBJS=	ifconfig.o ifconfig.sas.o

ifconfig.o:	ifconfig.c
ifconfig.sas.o:	ifconfig.sas.c
ifconfig:	ifconfig.o
ifconfig.sas:	ifconfig.sas.o

ifconfig.sas.c: ifconfig.c
	-rm -f ifconfig.sas.c
	cp ../ifconfig.c ifconfig.sas.c

ifconfig.sas.o:
	$(CCCMD) -DSTANDALONE ifconfig.sas.c
install:
	$(INSTALL) -c -s ifconfig $(DESTROOT)/bin/ifconfig
	$(INSTALL) -c -s ifconfig.sas $(DESTROOT)/etc/ifconfig.sas
	$(RM) $(DESTROOT)/etc/ifconfig
	$(LN) -s ../bin/ifconfig ${DESTROOT}/etc/ifconfig


include $(GMAKERULES)
