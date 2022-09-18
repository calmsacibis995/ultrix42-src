#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

DESTROOT=
CFLAGS=	-O
LIBS=	-lm -ltermcap
BIN=	$(DESTROOT)/usr/games
LIB=	$(DESTROOT)/usr/games/lib
USER=	daemon
UTILS=	snscore
OBJS=	snake.o move.o
DESTLIST= ${BIN} ${LIB}

all:	snake snscore	

snake:	$(OBJS)
	cc $(OBJS) -o snake $(LIBS)

snake.o :
	$(CC) -c ../snake.c

move.o:
	$(CC) -c ../move.c

snscore: snscore.c
	cc $(CFLAGS) ../snscore.c -o snscore

busy:	busy.c
	cc $(CFLAGS) busy.c -o busy

install:
	install -c -s -m 4755 -o ${USER} snake ${BIN}/snake
	install -c -s -m 755 -o ${USER} snscore ${BIN}/snscore
#	install -c -s -m 755 -o ${USER} busy ${BIN}/busy
	cat /dev/null >> $(LIB)/snakerawscores
	chmod 644 $(LIB)/snakerawscores
	/etc/chown $(USER) $(LIB)/snakerawscores
	install -c /dev/null ${LIB}/snake.log


include $(GMAKERULES)
