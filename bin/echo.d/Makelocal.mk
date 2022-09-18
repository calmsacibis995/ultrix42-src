#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin

AOUT=	echo

OBJS=	echo.o

echo.o:	echo.c

install:
	$(INSTALL) -c -s echo $(DESTROOT)/bin/echo

include $(GMAKERULES)
