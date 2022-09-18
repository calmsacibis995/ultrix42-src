# @(#)Makelocal.mk	4.1	ULTRIX	7/17/90

include $(GMAKEVARS)

CDEFINES=-DNEQN
CINCLUDES=-I. -I.. -I../../eqn.d -I$(SRCROOT)/usr/include
YFLAGS=-d

CFILES=	diacrit.c eqnbox.c font.c fromto.c funny.c glob.c integral.c \
	io.c lex.c lookup.c mark.c matrix.c move.c over.c paren.c \
	pile.c shift.c size.c sqrt.c text.c

OBJS1=	e.o

OBJS2=	diacrit.o eqnbox.o font.o fromto.o funny.o glob.o \
	integral.o io.o lex.o lookup.o mark.o matrix.o move.o over.o \
	paren.o pile.o shift.o size.o sqrt.o text.o

AOUT=	neqn

OBJS=	$(OBJS1) $(OBJS2)

e.o:	../../eqn.d/e.y
	$(YACC) $(YFLAGS) ../../eqn.d/e.y
	$(CCCMD) y.tab.c
	-$(RM) y.tab.c
	$(MV) y.tab.o $@
	$(MV) y.tab.h e.def


$(OBJS2):	../../eqn.d/e.h
	$(CC) -c $(CFLAGS) $(CDEFINES) $(CINCLUDES) $*.c

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

$(CFILES):
	$(RM) $@
	$(LN) -s ../../eqn.d/$@ $@

diacrit.c:	../../eqn.d/diacrit.c
eqnbox.c:	../../eqn.d/eqnbox.c
font.c:		../../eqn.d/font.c
fromto.c:	../../eqn.d/fromto.c
funny.c:	../../eqn.d/funny.c
glob.c:		../../eqn.d/glob.c
integral.c:	../../eqn.d/integral.c
io.c:		../../eqn.d/io.c
lex.c:		../../eqn.d/lex.c
lookup.c:	../../eqn.d/lookup.c
mark.c:		../../eqn.d/mark.c
matrix.c:	../../eqn.d/matrix.c
move.c:		../../eqn.d/move.c
over.c:		../../eqn.d/over.c
paren.c:	../../eqn.d/paren.c
pile.c:		../../eqn.d/pile.c
shift.c:	../../eqn.d/shift.c
size.c:		../../eqn.d/size.c
sqrt.c:		../../eqn.d/sqrt.c
text.c:		../../eqn.d/text.c

install:
	$(INSTALL) -c -s neqn $(DESTROOT)/usr/bin/neqn

include $(GMAKERULES)
