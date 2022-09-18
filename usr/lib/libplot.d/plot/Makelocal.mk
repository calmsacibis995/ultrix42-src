#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90
include $(GMAKEVARS)
OBJS=	arc.o box.o circle.o close.o cont.o dot.o erase.o label.o \
	line.o linmod.o move.o open.o point.o putsi.o space.o

all libplot:	${OBJS}
	ar cu libplot ${OBJS}

tools2 install:
	install -c -m 644 libplot ${DESTROOT}/usr/lib/libplot.a
	ranlib ${DESTROOT}/usr/lib/libplot.a

include $(GMAKERULES)

arc.o:	arc.c
box.o:	box.c
circle.o:	circle.c
close.o:	close.c
cont.o:	cont.c
dot.o:	dot.c
erase.o:	erase.c
label.o:	label.c
line.o:	line.c
linmod.o:	linmod.c
move.o:	move.c
open.o:	open.c
point.o:	point.c
putsi.o:	putsi.c
space.o:	space.c
