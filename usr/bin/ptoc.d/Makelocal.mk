#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90
#
#
include $(GMAKEVARS)
CFLAGS=-O
CINCLUDES= -I..
OBJS=main.o sc.o prsubr.o prdecl.o prstmt.o prexpr.o emit.o builtins.o
AOUT=ptoc

${OBJS}: ptoc.h
main.o:		main.c
sc.o:		sc.c
prsubr.o:	prsubr.c
prdecl.o:	prdecl.c
prstmt.o:	prstmt.c
prexpr.o:	prexpr.c
emit.o:		emit.c
builtins.o:	builtins.c

install:
	install -c -s ptoc ${DESTROOT}/usr/bin/ptoc

include $(GMAKERULES)
