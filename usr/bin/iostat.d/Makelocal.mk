#	@(#)Makelocal.mk	4.1	(ULTRIX)	7/17/90
include $(GMAKEVARS)
AOUT=iostat
OBJS=iostat.o
DESTDIR=usr/bin

iostat.o: iostat.c

install:
	install -c -m 2711 -g kmem -s iostat ${DESTROOT}/${DESTDIR}/iostat

include $(GMAKERULES)
