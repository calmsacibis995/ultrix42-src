#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90
include $(GMAKEVARS)
OBJS=   arc.o box.o circle.o close.o color.o cont.o dot.o erase.o label.o \
	line.o linemod.o move.o open.o point.o space.o

all liblvp16:	${OBJS}
	ar cu liblvp16 ${OBJS}

tools2 install:
	install -c -m 644 liblvp16 ${DESTROOT}/usr/lib/liblvp16.a
	ranlib ${DESTROOT}/usr/lib/liblvp16.a

${OBJS}:        lvp16.h

include $(GMAKERULES)

arc.o:	arc.c
box.o:	box.c
circle.o:	circle.c
close.o:	close.c
color.o:	color.c
cont.o:	cont.c
dot.o:	dot.c
erase.o:	erase.c
label.o:	label.c
line.o:	line.c
linemod.o:	linemod.c
move.o:	move.c
open.o:	open.c
point.o:	point.c
space.o:	space.c
