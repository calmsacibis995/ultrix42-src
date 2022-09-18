#  @(#)Makelocal.mk	4.1	ULTRIX	7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc

LOADLIBES = -loldbm 

AOUT= ypserv

OBJS = ypserv.o ypserv_ancil.o ypserv_map.o ypserv_proc.o

install:
	$(INSTALL) -s -c ypserv $(DESTROOT)/usr/etc/ypserv

ypserv.o:	ypserv.c
ypserv_ancil.o:	ypserv_ancil.c
ypserv_map.o:	ypserv_map.c
ypserv_proc.o:	ypserv_proc.c


include $(GMAKERULES)
