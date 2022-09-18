#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	ar

OBJS=	ar.o

ar.o:	ar.c

install:
	$(INSTALL) -s -c ar $(DESTROOT)/usr/bin/ar
	$(RM) $(DESTROOT)/bin/ar
	$(LN) -s ../usr/bin/ar $(DESTROOT)/bin/ar

include $(GMAKERULES)
