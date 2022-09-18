#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90
include $(GMAKEVARS)
OBJS=	arc.o box.o circle.o close.o dot.o erase.o label.o \
	line.o linmod.o move.o open.o point.o space.o subr.o

all lib300:	${OBJS}
	ar cu lib300 ${OBJS}

tools2 install:
	install -c -m 644 lib300 ${DESTROOT}/usr/lib/lib300.a
	ranlib ${DESTROOT}/usr/lib/lib300.a
	install -c ${DESTROOT}/usr/lib/lib300.a ${DESTROOT}/usr/lib/libt300.a

include $(GMAKERULES)

arc.o:	arc.c
box.o:	box.c
circle.o:	circle.c
close.o:	close.c
dot.o:	dot.c
erase.o:	erase.c
label.o:	label.c
line.o:	line.c
linmod.o:	linmod.c
move.o:	move.c
open.o:	open.c
point.o:	point.c
space.o:	space.c
subr.o:	subr.c
