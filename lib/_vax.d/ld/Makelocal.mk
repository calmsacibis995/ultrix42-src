# @(#)Makelocal.mk	4.1	(ULTRIX)	7/3/90
include $(GMAKEVARS)
AOUT=ld
OBJS=ld.o
CFLAGS=-O
LDFLAGS=-s

ld.o: ld.c

install:
	$(INSTALL) -c ld ${DESTROOT}/usr/bin/ld
	$(RM) $(DESTROOT)/bin/ld; \
	ln -s ../usr/bin/ld ${DESTROOT}/bin/ld

include $(GMAKERULES)
