#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin $(DESTROOT)/usr/bin

XSTR=	xstr

OBJSMISC=args.o blok.o cmd.o ctype.o defs.o echo.o error.o expand.o \
	fault.o func.o hash.o hashserv.o io.o macro.o main.o msg.o \
	name.o print.o pwd.o service.o setbrk.o stak.o string.o \
	sub.o test.o word.o xec.o

HFILES=	brkincr.h ctype.h defs.h dup.h hash.h mac.h mode.h name.h\
	stak.h sym.h timeout.h

all:	sh5
sh5:	$(OBJSMISC) strings.o
	$(CC) $(LDFLAGS) -o sh5 $(OBJSMISC) strings.o

args.o:		args.c
blok.o:		blok.c
cmd.o:		cmd.c
ctype.o:	ctype.c
defs.o:		defs.c
echo.o:		echo.c
error.o:	error.c
expand.o:	expand.c
fault.o:	fault.c
func.o:		func.c
hash.o:		hash.c
hashserv.o:	hashserv.c
io.o:		io.c
macro.o:	macro.c
main.o:		main.c
msg.o:		msg.c
name.o:		name.c
print.o:	print.c
pwd.o:		pwd.c
service.o:	service.c
setbrk.o:	setbrk.c
stak.o:		stak.c
string.o:	string.c
sub.o:		sub.c
test.o:		test.c
word.o:		word.c
xec.o:		xec.c

# string sharing
$(OBJSMISC): $(HFILES)
	$(CC) -E $(CFLAGS) ../$*.c | ${XSTR} -c -
	sed -f ../header < x.c > xtmp.c
	$(CC) -c $(CFLAGS) xtmp.c
	mv xtmp.o $*.o

# processed to fix n bit characters
strings.o: strings
	${XSTR}
	sed -f ../header < xs.c >xstmp.c
	mv xstmp.c xs.c
	$(CC) -c xs.c
	mv xs.o strings.o

install:
	$(INSTALL) -c -s sh5 $(DESTROOT)/usr/bin/sh5
	$(RM) $(DESTROOT)/bin/sh5
	$(LN) -s ../usr/bin/sh5 $(DESTROOT)/bin/sh5
	$(RM) $(DESTROOT)/usr/bin/rsh5
	$(LN) $(DESTROOT)/usr/bin/sh5 $(DESTROOT)/usr/bin/rsh5
	$(RM) $(DESTROOT)/bin/rsh5
	$(LN) -s ../usr/bin/rsh5 $(DESTROOT)/bin/rsh5

include $(GMAKERULES)
