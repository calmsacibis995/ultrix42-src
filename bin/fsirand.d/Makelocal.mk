#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin $(DESTROOT)/etc

AOUT=	fsirand

OBJS=	fsirand.o

fsirand.o:	fsirand.c

install:
	$(INSTALL) -c -s fsirand $(DESTROOT)/bin/fsirand
	$(RM) $(DESTROOT)/etc/fsirand
	$(LN) -s ../bin/fsirand ${DESTROOT}/etc/fsirand


include $(GMAKERULES)
