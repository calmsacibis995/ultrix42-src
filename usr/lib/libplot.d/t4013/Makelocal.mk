#  @(#)Makelocal.mk	4.1	ULTRIX	7/2/90

include $(GMAKEVARS)

OBJS=	arc.o box.o circle.o close.o dot.o erase.o label.o \
	line.o linemod.o move.o open.o point.o space.o subr.o

ARFILE=lib4013.a

install:
	$(INSTALL) -c -m 644 lib4013.a ${DESTROOT}/usr/lib/lib4013.a
	ranlib ${DESTROOT}/usr/lib/lib4013.a

arc.o:	arc.c
box.o:	box.c
circle.o:	circle.c
close.o:	close.c
dot.o:	dot.c
erase.o:	erase.c
label.o:	label.c
line.o:	line.c
linemod.o:	linemod.c
move.o:	move.c
open.o:	open.c
point.o:	point.c
space.o:	space.c
subr.o:	subr.c

include $(GMAKERULES)

