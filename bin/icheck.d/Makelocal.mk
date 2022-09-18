#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin $(DESTROOT)/etc

AOUT=	icheck

OBJS=	icheck.o

icheck.o:	icheck.c

install:
	$(INSTALL) -c -s icheck $(DESTROOT)/bin/icheck
	$(RM) $(DESTROOT)/etc/icheck
	$(LN) -s ../bin/icheck ${DESTROOT}/etc/icheck


include $(GMAKERULES)
