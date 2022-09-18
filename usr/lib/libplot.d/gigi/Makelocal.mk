#  @(#)Makelocal.mk	4.1	ULTRIX	7/2/90

include $(GMAKEVARS)

OBJS=	arc.o box.o circle.o close.o cont.o dot.o erase.o label.o \
	line.o linemod.o move.o open.o point.o space.o

ARFILE=libplotgigi.a

install:
	$(INSTALL) -c -m 644 libplotgigi.a ${DESTROOT}/usr/lib/libplotgigi.a
	ranlib ${DESTROOT}/usr/lib/libplotgigi.a

arc.o:	arc.c
box.o:	box.c
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
space.o:	space.c

include $(GMAKERULES)

