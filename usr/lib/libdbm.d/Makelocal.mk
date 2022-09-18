#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/lib

all:	libdbm.a

libdbm.a: dbm.c
	${CCCMD} ../dbm.c
	ar qc libdbm.a dbm.o
	ranlib libdbm.a

pretools tools1 tools2: libdbm.a
pretools tools1 tools2 install:
	install -c -m 644 libdbm.a $(DESTROOT)/usr/lib/libdbm.a
	ranlib $(DESTROOT)/usr/lib/libdbm.a

include $(GMAKERULES)
