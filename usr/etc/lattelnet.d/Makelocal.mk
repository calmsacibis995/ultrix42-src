# @(#)Makelocal.mk	4.1	(ULTRIX)	7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc

AOUT=	lattelnet

OBJS=	lattelnet.o

lattelnet.o : lattelnet.c 

install:
	$(INSTALL) -c -s lattelnet ${DESTROOT}/usr/etc/lattelnet
	
include $(GMAKERULES)
