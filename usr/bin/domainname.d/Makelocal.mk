#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

AOUT=	domainname

OBJS=	domainname.o

domainname:	domainname.o
domainname.o:	domainname.c

install:
	$(INSTALL) -c -s domainname $(DESTROOT)/usr/bin/domainname
	$(RM) $(DESTROOT)/bin/domainname
	$(LN) -s ../usr/bin/domainname $(DESTROOT)/bin/domainname

include $(GMAKERULES)
