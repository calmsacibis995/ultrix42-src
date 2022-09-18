#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

OBJS=	monop.o cards.o execute.o getinp.o houses.o jail.o misc.o \
	morg.o print.o prop.o rent.o roll.o spec.o strcmp.o trade.o \
	strings.o

DATFILES=brd.dat mon.dat prop.dat
LIBDIR=	$(DESTROOT)/usr/games/lib
BINDIR= $(DESTROOT)/usr/games
DAT=	brd.dat monop.dat prop.dat
XSTR=	xstr
ED=	ed
DESTLIST= $(BINDIR) $(LIBDIR)
#
# Be cool about compiling strings.
#

all:	monop cards.pck

monop:	$(OBJS)
	$(CC) -o monop -n $(OBJS) $(LIB)

install: monop cards.pck
	$(RM) $(BINDIR)/monop $(LIBDIR)/cards.pck
	strip monop
	install -c monop $(BINDIR)
	install -c cards.pck $(LIBDIR)/cards.pck


strings.o: strings
	$(XSTR)
	$(CC) -S xs.c
	$(ED) - < ../:rofix xs.s
	$(AS) -o strings.o xs.s
	$(RM) xs.s xs.c

monop.o: $(DATFILES) monop.def
	$(CC) -E $(CFLAGS) ../monop.c | $(XSTR) -c -
	$(CC) -c $(CFLAGS) x.c
	mv x.o monop.o

cards.o: deck.h
	$(CC) -E $(CFLAGS) ../cards.c | $(XSTR) -c -
	$(CC) -c $(CFLAGS) x.c
	mv x.o cards.o

cards.pck: initdeck cards.inp
	./initdeck

initdeck: initdeck.c deck.h
	${CC} -o initdeck ${CFLAGS} ../initdeck.c ${LIB}

execute.o :	execute.c
getinp.o :	getinp.c
houses.o :	houses.c
jail.o :	jail.c
misc.o :	misc.c
morg.o :	morg.c
print.o :	print.c
prop.o :	prop.c
rent.o :	rent.c
roll.o :	roll.c
spec.o :	spec.c
strcmp.o :	strcmp.c
trade.o :	trade.c



include $(GMAKERULES)
