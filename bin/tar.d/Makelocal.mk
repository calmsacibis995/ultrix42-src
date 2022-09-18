# @(#)Makelocal.mk	4.2	(ULTRIX)	12/6/90

include $(GMAKEVARS)

CFLAGS=
LDFLAGS=

DESTLIST=$(DESTROOT)/bin

OBJS=	tar.o command.o readtape.o writetape.o

AOUT=	tar

install:
	$(INSTALL) -c -s tar ${DESTROOT}/bin
	$(RM) ${DESTROOT}/bin/mdtar; \
	$(LN) -s tar ${DESTROOT}/bin/mdtar

tar.o:		tar.c
command.o:	command.c
readtape.o:	readtape.c
writetape.o:	writetape.c

include $(GMAKERULES)
