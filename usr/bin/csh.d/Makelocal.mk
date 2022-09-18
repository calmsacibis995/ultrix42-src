#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

CFLAGS=
CDEFINES=-DTELL -DVMUNIX -DVFORK -DTENEX -DCSHEDIT
LOADLIBES=-ltermcap

OBJS=	alloc.o printf.o sh.o sh.dir.o sh.dol.o sh.err.o sh.exec.o \
	sh.exp.o sh.func.o sh.glob.o sh.hist.o sh.init.o sh.lex.o \
	sh.misc.o sh.parse.o sh.print.o sh.proc.o sh.sem.o sh.set.o \
	sh.time.o doprnt.o sh.char.o tenex.o sh.editglue.o sh.edit.o

AOUT=	csh

install:
	$(INSTALL) -c -s csh $(DESTROOT)/usr/bin/csh
	$(RM) -f $(DESTROOT)/bin/csh
	$(LN) -s ../usr/bin/csh $(DESTROOT)/bin/csh

alloc.o: alloc.c
printf.o: printf.c
sh.o: sh.c
sh.dir.o: sh.dir.c
sh.dol.o: sh.dol.c
sh.err.o: sh.err.c
sh.exec.o: sh.exec.c
sh.exp.o: sh.exp.c
sh.func.o: sh.func.c
sh.glob.o: sh.glob.c
sh.hist.o: sh.hist.c
sh.init.o: sh.init.c
sh.lex.o: sh.lex.c
sh.misc.o: sh.misc.c
sh.parse.o: sh.parse.c
sh.print.o: sh.print.c
sh.proc.o: sh.proc.c
sh.sem.o: sh.sem.c
sh.set.o: sh.set.c
sh.time.o: sh.time.c
doprnt.o: doprnt.c
sh.char.o: sh.char.c
tenex.o: tenex.c
sh.editglue.o: sh.editglue.c
sh.edit.o: sh.edit.c

include $(GMAKERULES)
