#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90
                               
include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc

LOADLIBES= -lckrb -lkrb -lknet -ldes -lauth -ldbm


OBJS=	hesupd.o chpw_bsd.o chpw_trans.o chpw_c2.o 

AOUT= 	hesupd 


install:
	$(INSTALL) -s -c hesupd $(DESTROOT)/usr/etc/hesupd

hesupd:			hesupd.o chpw_bsd.o chpw_trans.o chpw_c2.o
hesupd.o:		hesupd.c
chpw_bsd.o: 		chpw_bsd.c
chpw_trans.o:		chpw_trans.c
chpw_c2.o: 		chpw_c2.c


include $(GMAKERULES)
