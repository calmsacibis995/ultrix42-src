#  @(#)Makelocal.mk	4.1	ULTRIX	7/2/90

include $(GMAKEVARS)

OBJS=	arc.o box.o circle.o clospl.o cont.o dot.o erase.o label.o \
	line.o linemd.o move.o open.o point.o space.o

ARFILE=libf77plot.a

install:
	$(INSTALL) -c -m 644 libf77plot.a ${DESTROOT}/usr/lib/libf77plot.a
	ranlib ${DESTROOT}/usr/lib/libf77plot.a

arc.o:	arc.c
box.o:	box.c
circle.o:	circle.c
clospl.o:	clospl.c
cont.o:	cont.c
dot.o:	dot.c
erase.o:	erase.c
label.o:	label.c
line.o:	line.c
linemd.o:	linemd.c
move.o:	move.c
open.o:	open.c
point.o:	point.c
space.o:	space.c

include $(GMAKERULES)

