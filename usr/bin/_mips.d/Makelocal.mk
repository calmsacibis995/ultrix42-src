# @(#)Makelocal.mk	4.1	(ULTRIX)	7/17/90

include $(GMAKEVARS)

AOUTS=uac

OBJS=uac.o

uac:	uac.o
uac.o:	uac.c

install:
	$(INSTALL) -c -s uac ${DESTROOT}/usr/bin/uac

include $(GMAKERULES)
