#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin $(DESTROOT)/etc

AOUT=	halt

OBJS=	halt.o

halt.o:	halt.c

install:
	$(INSTALL) -c -s halt $(DESTROOT)/bin/halt
	$(RM) $(DESTROOT)/etc/halt
	$(LN) -s ../bin/halt $(DESTROOT)/etc/halt

include $(GMAKERULES)
