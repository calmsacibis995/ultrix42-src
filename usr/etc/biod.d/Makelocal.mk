#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	biod

OBJS=	biod.o

biod.o:	biod.c

install:
	$(INSTALL) -c -s biod $(DESTROOT)/usr/etc/biod
	$(RM) $(DESTROOT)/etc/biod
	$(LN) -s ../usr/etc/biod $(DESTROOT)/etc/biod

include $(GMAKERULES)
