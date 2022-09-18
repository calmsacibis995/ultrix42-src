#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	id

OBJS=	id.o

id:	id.o
id.o:	id.c

install:
	$(INSTALL) -c -s id $(DESTROOT)/usr/bin/id
	$(RM) $(DESTROOT)/bin/id
	$(LN) -s ../usr/bin/id $(DESTROOT)/bin/id

include $(GMAKERULES)
