#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)


DESTLIST= $(DESTROOT)/usr/games

OBJS=	allow.o board.o check.o extra.o fancy.o init.o main.o move.o \
	odds.o one.o save.o subs.o table.o text.o message.o

TOBJS=	allow.o board.o check.o data.o fancy.o init.o odds.o one.o \
	save.o subs.o table.o teach.o ttext1.o ttext2.o tutor.o

 
allow.o:	allow.c
board.o:	board.c
check.o:	check.c
data.o:		data.c
extra.o:	extra.c
fancy.o :	fancy.c 
init.o :	init.c 
odds.o:		odds.c
one.o:		one.c
save.o:		save.c
subs.o:		subs.c
table.o:	table.c
teach.o:	teach.c
text.o:		text.c
ttext1.o:	ttext1.c
ttext2.o:	ttext2.c
tutor.o:	tutor.c
main.o :	main.c 
message.o:	message.c
move.o:		move.c

all:	backgammon teachgammon
	
backgammon: $(OBJS)
	$(CC) -o backgammon $(OBJS) -ltermlib

teachgammon: $(TOBJS)
	$(CC) -o teachgammon $(TOBJS) -ltermlib

install:
	$(INSTALL) -c -s backgammon $(DESTROOT)/usr/games/backgammon
	$(INSTALL) -c -s teachgammon $(DESTROOT)/usr/games/teachgammon


include $(GMAKERULES)
