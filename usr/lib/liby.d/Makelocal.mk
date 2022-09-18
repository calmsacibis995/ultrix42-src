#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

ARFILE=	liby.a

OBJS=	main.o yyerror.o

main.o:		main.c
yyerror.o:	yyerror.c

install:
	install -c -m 644 liby.a ${DESTROOT}/usr/lib
	ranlib ${DESTROOT}/usr/lib/liby.a

include $(GMAKERULES)
