#	@(#)Makelocal.mk	4.1	(ULTRIX)	7/17/90
include $(GMAKEVARS)
AOUT=ipcs
OBJS=ipcs.o

ipcs.o: ipcs.c

install:
	install -c -m 2711 -g kmem -s ipcs ${DESTROOT}/usr/bin/ipcs

include $(GMAKERULES)
