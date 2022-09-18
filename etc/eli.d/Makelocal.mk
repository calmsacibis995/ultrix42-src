#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST= $(DESTROOT)/etc

AOUT=	eli

LOADLIBES= -lerrlog

OBJS=	eli.o

eli.o:	eli.c

install:
	$(INSTALL) -c -s eli $(DESTROOT)/etc/eli


include $(GMAKERULES)
