# @(#)Makefile	4.1	ULTRIX	11/23/87
#
# @(#)Makefile	4.4 (Berkeley) 10/13/82

DESTROOT	=
CFLAGS=	-O -I.

OBJS=	closedir.o opendir.o readdir.o seekdir.o telldir.o

.c.o:
	${CC} ${CFLAGS} -c $*.c
	-ld -x -r $*.o
	mv a.out $*.o

libndir: ${OBJS}
	ar cru libndir ${OBJS}
	ranlib libndir

install: libndir
	cp dir.h ${DESTROOT}/usr/include/dir.h
	cp libndir ${DESTROOT}/usr/lib/libndir.a
	ranlib ${DESTROOT}/usr/lib/libndir.a
	cp directory.3s /usr/man/man3/directory.3s

clean:
	rm -f libndir ${OBJS}

clobber: clean
	rm -f *.[ch] directory.3s Makefile

sccsinfo:
	sccs info

sccsget:
	sccs get SCCS

.DEFAULT:
	sccs get $<
