#@(#)Makefile	4.1	Ultrix	7/17/90
# $Header: Makefile,v 1.5 84/05/19 11:42:13 powell Exp $

DESTROOT=
INSTALLDIR = ${DESTROOT}/usr/bin
CFLAGS = #-g
PFLAGS = -w #-C -g
OBJS =	p2m2.o \
	y.tab.o stringtab.o keywords.o scanner.o \
	error.o io.o util.o

p2m2:	$(OBJS)
	pc ${PFLAGS} -o p2m2 $(OBJS)

error.o: error.p stringtab.h tokens.h globals.h error.h scanner.h
keywords.o: keywords.p stringtab.h tokens.h globals.h keywords.h
p2m2.o: p2m2.p tokens.h stringtab.h globals.h error.h scanner.h io.h\
	keywords.h util.h
scanner.o: scanner.p tokens.h stringtab.h globals.h error.h scanner.h io.h\
	keywords.h util.h
stringtab.o: stringtab.p stringtab.h tokens.h error.h globals.h util.h
util.o: util.p tokens.h stringtab.h error.h globals.h scanner.h util.h
io.o:	io.c io.h

y.tab.c: pascal.gram
	@echo should produce 3 shift/reduce conflicts
	yacc -vd pascal.gram

y.tab.o: y.tab.c

install:	p2m2
	cp p2m2 $(INSTALLDIR)
clean:
	-rm -f *.o y.tab.c y.tab.h y.output
