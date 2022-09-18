#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

DESTLIST= $(DESTROOT)/usr/games

AOUT= mille

OBJS=	comp.o end.o extern.o init.o mille.o misc.o move.o print.o \
	roll.o save.o types.o varpush.o

LOADLIBES=	-lcurses -ltermlib

comp.o :	comp.c
end.o :		end.c
extern.o :	extern.c
init.o :	init.c
mille.o :	mille.c
misc.o :	misc.c
move.o :	move.c
print.o :	print.c
roll.o :	roll.c
save.o :	save.c
types.o :	types.c
varpush.o:	varpush.c

install: 
	$(INSTALL) -c -s mille $(DESTROOT)/usr/games/mille

include $(GMAKERULES)
