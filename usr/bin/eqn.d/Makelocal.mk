# @(#)Makelocal.mk	4.1	ULTRIX	7/17/90

include $(GMAKEVARS)

YFLAGS=-d

OBJS=	e.o diacrit.o eqnbox.o font.o fromto.o funny.o glob.o \
	integral.o io.o lex.o lookup.o mark.o matrix.o move.o over.o \
	paren.o pile.o shift.o size.o sqrt.o text.o

AOUT=	eqn

e.o:	e.y e.h
	$(YACC) $(YFLAGS) ../$<
	$(CCCMD) y.tab.c
	-$(RM) y.tab.c
	$(MV) y.tab.o $@
	mv y.tab.h e.def

diacrit.o:	diacrit.c
eqnbox.o:	eqnbox.c
font.o:		font.c
fromto.o:	fromto.c
funny.o:	funny.c
glob.o:		glob.c
integral.o:	integral.c
io.o:		io.c
lex.o:		lex.c
lookup.o:	lookup.c
mark.o:		mark.c
matrix.o:	matrix.c
move.o:		move.c
over.o:		over.c
paren.o:	paren.c
pile.o:		pile.c
shift.o:	shift.c
size.o:		size.c
sqrt.o:		sqrt.c
text.o:		text.c

install:
	$(INSTALL) -c -s eqn $(DESTROOT)/usr/bin/eqn

include $(GMAKERULES)
