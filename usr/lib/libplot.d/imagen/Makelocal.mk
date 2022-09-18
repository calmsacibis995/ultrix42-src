#  @(#)Makelocal.mk	4.1	ULTRIX	7/2/90

include $(GMAKEVARS)

OBJS=	arc.o box.o charset.o circle.o close.o cont.o dot.o erase.o label.o \
	line.o linemod.o move.o open.o point.o scale.o space.o

CFLAGS= -O -DDEFRES=240

ARFILE=libplotimagen.a

install:
	$(INSTALL) -c -m 644 libplotimagen.a ${DESTROOT}/usr/lib/libplotimagen.a
	ranlib ${DESTROOT}/usr/lib/libplotimagen.a

arc.o:	arc.c
box.o:	box.c
charset.o:	charset.c
circle.o:	circle.c
close.o:	close.c
cont.o:	cont.c
dot.o:	dot.c
erase.o:	erase.c
label.o:	label.c
line.o:	line.c
linemod.o:	linemod.c
move.o:	move.c
open.o:	open.c
point.o:	point.c
scale.o:	scale.c
space.o:	space.c

include $(GMAKERULES)

