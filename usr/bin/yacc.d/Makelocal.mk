# @(#)Makelocal.mk	4.1	ULTRIX 	7/17/90
include $(GMAKEVARS)
#	@(#)Makefile	4.2	(Berkeley)	83/02/11
CFLAGS=-O -DWORD32
CINCLUDES= -I..
SRCS = Makefile dextern files yaccpar \
	y1.c y2.c y3.c y4.c \
	yaccdiffs yaccnews

all: yacc

y1.o y2.o y3.o y4.o:
	$(CCCMD) ../$(@:.o=.c)

yacc: y1.o y2.o y3.o y4.o
	$(LDCMD) y?.o 
y1.o: dextern files yaccpar.h
y2.o y3.o y4.o: dextern files 

pretools tools1 tools2: all
pretools tools1 tools2 install:
	install -c -s yacc $(DESTROOT)/usr/bin/yacc
	install -c ../yaccpar $(DESTROOT)/usr/lib/yaccpar

y1.o:	../y1.c
y2.o:	../y2.c
y3.o:	../y3.c
y4.o:	../y4.c
include $(GMAKERULES)
