#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin $(DESTROOT)/etc

AOUT=	tunefs

OBJS=	tunefs.o

tunefs.o:	tunefs.c

install:
	$(INSTALL) -c -s tunefs $(DESTROOT)/bin/tunefs
	$(RM) $(DESTROOT)/etc/tunefs
	$(LN) -s ../bin/tunefs ${DESTROOT}/etc/tunefs


include $(GMAKERULES)
