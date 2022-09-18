#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	nfsd

OBJS=	nfsd.o

nfsd.o:	nfsd.c

install:
	$(INSTALL) -c -s nfsd $(DESTROOT)/usr/etc/nfsd
	$(RM) $(DESTROOT)/etc/nfsd
	$(LN) -s ../usr/etc/nfsd $(DESTROOT)/etc/nfsd

include $(GMAKERULES)
