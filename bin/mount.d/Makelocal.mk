#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin $(DESTROOT)/etc

AOUT=	mount

OBJS=	mount.o

mount.o:	mount.c

install:
	$(INSTALL) -c -s mount $(DESTROOT)/bin/mount
	$(RM) $(DESTROOT)/etc/mount
	$(LN) -s ../bin/mount $(DESTROOT)/etc/mount

include $(GMAKERULES)
