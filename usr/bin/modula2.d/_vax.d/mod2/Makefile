#@(#)Makefile	4.1	Ultrix	7/17/90
# $Header: Makefile,v 1.5 84/05/19 11:34:45 powell Exp $

DESTROOT=
MODLIBDIR = ${DESTROOT}/usr/lib/mod

CFLAGS = #-g
PFLAGS = -w #-C -g
TOBJS =	gent.o mahler.o builtint.o mod2.o\
	y.tab.o stringtab.o keywdtab.o scanner.o\
	symtab.o alloc.o\
	decls.o const.o bexpr.o bstmt.o\
	cstmt.o cexpr.o\
	optim.o ocount.o otree.o\
	builtin.o\
	error.o iolib.o machine.o memlib.o

OBJS =	mod2.o\
	y.tab.o stringtab.o keywdtab.o scanner.o\
	symtab.o alloc.o\
	decls.o const.o bexpr.o bstmt.o\
	cstmt.o cexpr.o\
	optim.o ocount.o otree.o\
	genpc.o pcode.o dbstab.o\
	builtin.o builtinpc.o\
	error.o iolib.o machine.o memlib.o

ALLH =	globals.h error.h tokens.h machine.h stringtab.h symtab.h

all: mod2

mod2:	$(OBJS)
	pc ${PFLAGS} -o mod2 $(OBJS)

mod2t:	$(TOBJS)
	pc ${PFLAGS} -o mod2t $(TOBJS)

mod2.o:		mod2.p iolib.h keywdtab.h scanner.h \
		decls.h genpc.h builtin.h \
		alloc.h optim.h $(ALLH)
error.o:	error.p scanner.h $(ALLH)
machine.o:	machine.p $(ALLH)

stringtab.o:	stringtab.p $(ALLH)
keywdtab.o:	keywdtab.p keywdtab.h $(ALLH)
scanner.o:	scanner.p scanner.h keywdtab.h iolib.h $(ALLH)

symtab.o:	symtab.p alloc.h decls.h $(ALLH)
decls.o:	decls.p decls.h scanner.h $(ALLH)
alloc.o:	alloc.p alloc.h $(ALLH)

const.o:	const.p const.h decls.h scanner.h $(ALLH)
bexpr.o:	bexpr.p bexpr.h const.h scanner.h $(ALLH)
bstmt.o:	bstmt.p bstmt.h bexpr.h $(ALLH)
cexpr.o:	cexpr.p cexpr.h bexpr.h const.h decls.h builtin.h alloc.h \
		$(ALLH)
cstmt.o:	cstmt.p cstmt.h bexpr.h cexpr.h const.h decls.h \
		alloc.h $(ALLH)

optim.o:	optim.p optim.h otree.h ocount.h const.h bexpr.h cexpr.h \
		pcode.h genpc.h builtin.h $(ALLH)
otree.o:	otree.p otree.h optim.h ocount.h genpc.h bexpr.h const.h \
		builtin.h $(ALLH)
ocount.o:	ocount.p ocount.h optim.h bexpr.h genpc.h builtin.h $(ALLH)

genpc.o:	genpc.p genpc.h pcode.h pcodeops.h optim.h decls.h bexpr.h \
		cexpr.h alloc.h const.h $(ALLH)
pcode.o:	pcode.p pcode.h pcodeops.h decls.h $(ALLH)
dbstab.o:	dbstab.p dbstab.h genpc.h pcode.h pcodeops.h optim.h ocount.h\
		decls.h $(ALLH)
builtin.o:	builtin.p builtin.h const.h bexpr.h cexpr.h decls.h genpc.h\
		pcode.h optim.h ocount.h otree.h $(ALLH)
builtinpc.o:	builtinpc.p builtin.h const.h bexpr.h cexpr.h decls.h genpc.h\
		pcode.h optim.h ocount.h otree.h $(ALLH)
builtint.o:	builtint.p builtin.h optim.h ocount.h gent.h mahler.h decls.h\
		$(ALLH)
mahler.o:	mahler.p mahler.h decls.h $(ALLH)
gent.o:		gent.p gent.h optim.h ocount.h gencode.h builtint.h alloc.h\
		mahler.h decls.h bexpr.h cexpr.h const.h $(ALLH)

iolib.o:	iolib.c iolib.h
memlib.o:	memlib.c
	cc -c -O ${CFLAGS} memlib.c

y.tab.o: y.tab.c

y.tab.c: mod2.gram
	@echo Should be 1 shift/reduce and 13 reduce/reduce conflicts
	yacc mod2.gram
install: mod2
	cp mod2 $(MODLIBDIR)/mod2.0
clean:
	-rm -f *.o y.tab.c y.output
