#  @(#)Makelocal.mk	4.1	ULTRIX	7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/var/yp $(DESTROOT)/etc

AOUT=	revnetgroup

OBJS=	revnetgroup.o getgroup.o table.o util.o

install:
	$(INSTALL) -s -c revnetgroup $(DESTROOT)/usr/var/yp/revnetgroup
	$(RM) $(DESTROOT)/etc/revnetgroup
	$(LN) -s ../usr/var/yp/revnetgroup $(DESTROOT)/etc/revnetgroup

revnetgroup.o:	revnetgroup.c
getgroup.o:	getgroup.c
table.o:	table.c
util.o:		util.c

include $(GMAKERULES)
