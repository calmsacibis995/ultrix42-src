# 		@(#)Makelocal.mk	4.1	ULTRIX	7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin

AOUT=	sh

OBJS=	args.o blok.o builtin.o cmd.o ctype.o error.o expand.o \
	fault.o io.o macro.o main.o msg.o name.o print.o service.o \
	setbrk.o stak.o string.o word.o xec.o

install:
	$(INSTALL) -c -s sh $(DESTROOT)/bin/sh

blok.o:		brkincr.h
fault.o:	brkincr.h
main.o:		brkincr.h
stak.o:		brkincr.h

args.o:		args.c
blok.o:		blok.c
builtin.o:	builtin.c
cmd.o:		cmd.c
ctype.o:	ctype.c
error.o:	error.c
expand.o:	expand.c
fault.o:	fault.c
io.o:		io.c
macro.o:	macro.c
main.o:		main.c
msg.o:		msg.c
name.o:		name.c
print.o:	print.c
service.o:	service.c
setbrk.o:	setbrk.c
stak.o:		stak.c
string.o:	string.c
word.o:		word.c
xec.o:		xec.c

include $(GMAKERULES)
