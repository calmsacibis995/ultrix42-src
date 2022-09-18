#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin $(DESTROOT)/etc

AOUT=	chpt

OBJS=	chpt.o

chpt.o:	chpt.c

install:
	$(INSTALL) -c -s chpt $(DESTROOT)/bin/chpt
	$(RM) $(DESTROOT)/etc/chpt
	$(LN) -s ../bin/chpt ${DESTROOT}/etc/chpt


include $(GMAKERULES)
