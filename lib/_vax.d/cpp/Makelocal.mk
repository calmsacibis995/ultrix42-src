# @(#)Makelocal.mk	4.1	(ULTRIX)	7/3/90

include $(GMAKEVARS)

CDEFINES=-Dunix=1 -Dbsd4_2=1 -Dultrix=1 -DFLEXNAMES
CFLAGS=	-O
YYFIX=	../../pcc/:yyfix

AOUT=	cpp

OBJS=	cpp.o cpy.o rodata.o

cpp.o:	cpp.c
cpy.o: 	cpy.y yylex.c
	$(YACC) ../cpy.y
	-@if [ -f $(YYFIX) -a -w $(YYFIX) ]; then \
		$(CHMOD) 555 $(YYFIX); \
	fi
	/bin/sh $(YYFIX) yyexca yyact yypact yypgo yyr1 yyr2 yychk yydef
	$(MV) y.tab.c cpy.c
	$(CCCMD) cpy.c

rodata.o: cpy.y
	$(CC) $(CFLAGS) -R -c rodata.c

install: cpp
	$(INSTALL) -s -c cpp $(DESTROOT)/usr/lib/cpp

include $(GMAKERULES)
