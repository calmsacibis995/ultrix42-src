# @(#)Makelocal.mk	4.1	ULTRIX	7/17/90
#
include $(GMAKEVARS)

CFLAGS = -O -w
CINCLUDES =-I. -I..

OBJS = main.o init.o tailor.o $(gram.o lex.o) symtab.o\
	dcl.o addr.o struct.o field.o blklab.o\
	mk.o misc.o alloc.o print.o simple.o exec.o temp.o io.o error.o\
	pass2.o icfile.o free.o dclgen.o namgen.o
OBJSMISC = gram.o lex.o

all: efl
efl : $(OBJS) $(OBJSMISC)
	$(LDCMD) $(CFLAGS) $(OBJS) $(OBJSMISC)

$(OBJS) :  ../defs
lex.o init.o : tokdefs

# gram.c can no longer be made on a pdp11 because of yacc limits
gram.c:	../gram.head ../gram.dcl ../gram.expr ../gram.exec tokdefs
	( sed <tokdefs "s/#define/%token/" ;\
	cat ../gram.head ../gram.dcl ../gram.expr ../gram.exec) >gram.in
	$(YACC) $(YFLAGS) gram.in
	(echo "#define YYDEBUG"; cat y.tab.c) > gram.c
	rm -f gram.in y.tab.c

gram.o:	gram.c
	$(CCCMD) gram.c

lex.c: ../fixuplex
	lex ../lex.l
	sh ../fixuplex
	mv lex.yy.c lex.c

lex.o:	lex.c
	$(CCCMD) lex.c

tokdefs: ../tokens
	grep -n . <../tokens | sed "s/\([^:]*\):\(.*\)/#define \2 \1/" >tokdefs

main.o:	main.c
init.o:	init.c
tailor.o:	tailor.c
symtab.o:	symtab.c
dcl.o:	dcl.c
addr.o:	addr.c
struct.o:	struct.c
field.o:	field.c
blklab.o:	blklab.c
mk.o:	mk.c
misc.o:	misc.c
alloc.o:	alloc.c
print.o:	print.c
simple.o:	simple.c
exec.o:	exec.c
temp.o:	temp.c
io.o:	io.c
error.o:	error.c
pass2.o:	pass2.c
icfile.o:	icfile.c
free.o:	free.c
dclgen.o:	dclgen.c
namgen.o:	namgen.c

lint: efl
	lint -p *.c -lS

install:
	install -c -s efl ${DESTROOT}/usr/bin/efl

efltest:	a.out
	a.out "system=gcos" efltest/Hard.e >z1 2>z2
	cmp z1 efltest/Hard.out
	a.out "system=gcos" efltest/Band.e >z1 2>z2
	cmp z1 efltest/Band.out
	a.out "system=gcos" efltest/Buram.e >z1 2>z2
	cmp z1 efltest/Buram.out
	a.out "system=gcos" efltest/Dgl.e >z1 2>z2
	cmp z1 efltest/Dgl.out
	rm -f z1 z2
	@echo TEST OK

include $(GMAKERULES)
