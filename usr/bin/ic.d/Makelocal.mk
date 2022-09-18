# @(#)Makelocal.mk	4.1	(ULTRIX)	7/17/90
# Makefile for ic, the I18N language database compiler.
#
#	20th Jan 88 - Created Martin Hills, EUEG.
#	
include $(GMAKEVARS)

CINCLUDES =  -I. -I.. -I${DESTROOT}/usr/include 
CDEFINES = -DULTRIX  
CFLAGS  = -O

OBJS	= cod.o col.o cnv.o frm.o io.o message.o \
	  main.o prp.o subr.o sym.o yyerror.o yywhere.o val.o
# The order of these two objects (OBJSMISC) is important!
OBJSMISC = y.tab.o lex.yy.o 

all : ic

ic : main.o ${OBJS} $(OBJSMISC)
	$(LDCMD) $(OBJSMISC) ${OBJS} -ll

${OBJS} $(OBJSMISC) main.o : ../ic.h ../dbg.h

y.tab.o: ../ic.y ../ic.h
	yacc -d ../ic.y
	/bin/sh ../xtoken
	sed -f ../sedscript <y.tab.c > tmp$$$$ && mv tmp$$$$ y.tab.c
	$(CCCMD) y.tab.c
	$(RM) y.tab.c

lex.yy.o: ../ic.l ../ic.h y.tab.h
	lex ../ic.l
	$(CCCMD) lex.yy.c
	$(RM) lex.yy.c

sym.o: y.tab.h

tools2:	ic
tools2 install:
	install -c -s -m 555 -o bin -g system ic ${DESTROOT}/usr/bin

cod.o:	cod.c
col.o:	col.c
cnv.o:	cnv.c
frm.o:	frm.c
io.o:	io.c
message.o:	message.c
main.o:	main.c
prp.o:	prp.c
subr.o:	subr.c
sym.o:	sym.c
yyerror.o:	yyerror.c
yywhere.o:	yywhere.c
val.o:	val.c

include $(GMAKERULES)
