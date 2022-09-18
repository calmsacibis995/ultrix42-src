# @(#)Makelocal.mk	4.1	(ULTRIX)	7/17/90
include $(GMAKEVARS)
#
CFLAGS=	-O 
CDEFINES=-DASCARCH
CINCLUDES=-I. -I..
AOUT=make
OBJS=ident.o main.o doname.o misc.o files.o dosys.o
OBJSMISC=gram.o

make:	gram.o

gram.o: gram.y
	$(YACC) $(YFLAGS) ../gram.y
	$(CCCMD) y.tab.c
	$(RM) y.tab.c
	mv y.tab.o gram.o

${OBJS}:  defs

pretools tools1 tools2: make
pretools tools1 tools2 install:
	install -c -s make ${DESTROOT}/usr/bin/make
	rm -f ${DESTROOT}/bin/make
	ln -s ../usr/bin/make ${DESTROOT}/bin/make


ident.o:	ident.c
main.o:		main.c
doname.o:	doname.c
misc.o:		misc.c
files.o:	files.c
dosys.o:	dosys.c

include $(GMAKERULES)
