#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/lib/learn/C

OBJS=	getline.o getnum.o

getline.o:	getline.c
getnum.o:	getnum.c

install:
	$(CP) ../L* $(DESTROOT)/usr/lib/learn/C
	$(INSTALL) -c -m 644 getline.o $(DESTROOT)/usr/lib/learn/C/getline.o
	$(INSTALL) -c -m 644 ../getline.c $(DESTROOT)/usr/lib/learn/C/getline.c
	$(INSTALL) -c -m 644 getnum.o $(DESTROOT)/usr/lib/learn/C/getnum.o
	$(INSTALL) -c -m 644 ../getnum.c $(DESTROOT)/usr/lib/learn/C/getnum.c

include $(GMAKERULES)
