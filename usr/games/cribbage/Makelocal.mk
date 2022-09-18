#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/games/lib

OBJS=	extern.o crib.o support.o cards.o score.o io.o

TOBJS=	test.o cards.o score.o io.o extern.o


all: cribbage crib.instr

cribbage:	$(OBJS)
	$(CC) $(CFLAGS) -o cribbage $(OBJS) -lcurses -ltermlib

crib.instr: cribbage.n macro
	nroff ../cribbage.n > crib.instr

tags: ${HDRS} ${CFILES}
	ctags -u $?
	ed - tags < :ctfix
	sort tags -o tags

test:		${TOBJS}
	${CC} ${CFLAGS} -o test ${TOBJS} -lcurses -ltermlib

crib.o io.o support.o: cribcur.h

crib.o:		deck.h	cribbage.h cribcur.h crib.c
support.o:	deck.h	cribbage.h cribcur.h support.c
test.o:		deck.h test.c
cards.o:	deck.h cards.c
score.o:	deck.h score.c
io.o:		deck.h io.c	cribcur.h
extern.o:	extern.c

install: cribbage crib.instr
	$(INSTALL) -c -s cribbage $(DESTROOT)/usr/games/cribbage
	$(INSTALL) -c crib.instr $(DESTROOT)/usr/games/lib/crib.instr

include $(GMAKERULES)

