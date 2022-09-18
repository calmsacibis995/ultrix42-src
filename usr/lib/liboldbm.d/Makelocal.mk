#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/lib

all:	liboldbm.a
	@echo liboldbm.a only gets put in to the build environment.
	install -c -m 644 liboldbm.a /usr/lib/liboldbm.a
	ranlib /usr/lib/liboldbm.a

liboldbm.a: dbm.c
	${CCCMD} ../dbm.c
	ar qc liboldbm.a dbm.o
	ranlib liboldbm.a

install:
	@echo Nothing to install.

include $(GMAKERULES)
