#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

#	@(#)Makefile	2.1	(Berkeley)	12/10/85
# optional flags are: MEASURE TESTING DEBUG

include $(GMAKEVARS)

all:	machine.o ops.o symtab.o stacktrace.o getu.o sysvad.o

machine.o:	machine.c
ops.o:	ops.c
symtab.o:	symtab.c
stacktrace.o:	 stacktrace.c
getu.o:	getu.c
sysvad.o:	sysvad.c

include $(GMAKERULES)

