#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90
include $(GMAKEVARS)
CFLAGS=-O

all: libln.a

libln.a:
	$(CCCMD) ../allprint.c ../main.c ../reject.c ../yyless.c
	$(CCCMD) ../yywrap.c
	rm -f libln.a
	ar rvc libln.a allprint.o main.o reject.o yyless.o yywrap.o
	rm allprint.o main.o reject.o yyless.o yywrap.o

pretools tools1 tools2: all
pretools tools1 tools2 install:
	install -c libln.a ${DESTROOT}/usr/lib
	rm -f ${DESTROOT}/usr/lib/libl.a
	ln ${DESTROOT}/usr/lib/libln.a ${DESTROOT}/usr/lib/libl.a
	ranlib ${DESTROOT}/usr/lib/libln.a

include $(GMAKERULES)
