#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin $(DESTROOT)/etc

AOUT=	newfs

OBJS=	newfs.o

newfs.o:	newfs.c

install:
	$(INSTALL) -c -s newfs $(DESTROOT)/bin/newfs
	$(RM) $(DESTROOT)/etc/newfs
	$(LN) -s ../bin/newfs $(DESTROOT)/etc/newfs

include $(GMAKERULES)
