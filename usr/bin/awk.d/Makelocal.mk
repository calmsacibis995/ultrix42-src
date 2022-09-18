# @(#)Makelocal.mk	4.1	(ULTRIX)	7/17/90
include $(GMAKEVARS)

YFLAGS=-d

SOURCE=../awk.def ../awk.g.y ../awk.lx.l ../b.c ../lib.c ../main.c \
	../parse.c ../proc.c ../freeze.c ../run.c ../token.c ../tran.c

# Need local copy because tokenscript edits it
# Must have name of token.c
# The path avoids rm of ./token.c if make is somehow run in source dir
TOKENLOCAL=../_$(MACHINE).b/token.c


OBJS=b.o main.o tran.o lib.o run.o parse.o freeze.o
b.o:		b.c
main.o:		main.c
tran.o:		tran.c
lib.o:		lib.c
run.o:		run.c
parse.o:	parse.c
freeze.o:	freeze.c

proctab.o:	proctab.c
	$(CCCMD) proctab.c

proc.o:		../proc.c
	$(CCCMD) ../proc.c

all:	awk
awk:	$(OBJS) token.o awk.lx.o awk.g.o proctab.o
	$(LDCMD) awk.g.o  awk.lx.o $(OBJS) token.o proctab.o -lm

y.tab.h:	awk.g.o
awk.g.o:	../awk.g.y
	$(YACC) $(YFLAGS) ../awk.g.y
	$(CCCMD) y.tab.c
	$(RM) y.tab.c
	mv y.tab.o awk.g.o

awk.lx.o: ../awk.lx.l
	$(LEX) $(LFLAGS) ../awk.lx.l
	$(CCCMD) lex.yy.c
	$(RM) lex.yy.c
	$(MV) lex.yy.o $@

awk.h:	y.tab.h
	-cmp -s y.tab.h awk.h || cp y.tab.h awk.h

$(OBJS) proc.o proctab.o awk.lx.o:	awk.h awk.def

token.o:	$(TOKENLOCAL)
#tokenscript edits token.c
token.o $(TOKENLOCAL):	../token.c awk.h
	cp ../token.c $(TOKENLOCAL)
	chmod 664 $(TOKENLOCAL)
	ex - <../tokenscript
	$(RM) temp
	$(CCCMD) $(TOKENLOCAL)

proctab.c:	proc
	proc >proctab.c

proc:	awk.h proc.o token.o
	$(LDCMD) proc.o token.o


pretools tools1 tools2: awk
pretools tools1 tools2 install:
	install -c -s awk ${DESTROOT}/bin

#rules below may need changing to get to work again
profile:	awk.g.o $(FILES) mon.o
	cc -p -i awk.g.o $(FILES) mon.o -lm

find:
	egrep -n "$(PAT)" *.[ylhc] awk.def

list:
	-pr $(SOURCE) ../Makefile ../tokenscript ../README ../EXPLAIN

lint:
	lint -spu b.c main.c token.c tran.c run.c lib.c parse.c -lm |\
		egrep -v '^(error|free|malloc)'

diffs:
	-for i in $(SOURCE); do echo $$i:; diff $$i /usr/src/cmd/awk | ind; done
include $(GMAKERULES)
