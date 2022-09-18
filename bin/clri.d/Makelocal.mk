#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin $(DESTROOT)/etc

AOUT=	clri

OBJS=	clri.o

clri.o:	clri.c

install:
	$(INSTALL) -c -s clri $(DESTROOT)/bin/clri
	$(RM) $(DESTROOT)/etc/clri
	$(LN) -s ../bin/clri ${DESTROOT}/etc/clri


include $(GMAKERULES)
