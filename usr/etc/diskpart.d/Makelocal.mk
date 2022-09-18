#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	diskpart

OBJS=	diskpart.o

diskpart.o:	diskpart.c

install:
	$(INSTALL) -c -s diskpart $(DESTROOT)/usr/etc/diskpart
	$(RM) $(DESTROOT)/etc/diskpart
	$(LN) -s ../usr/etc/diskpart $(DESTROOT)/etc/diskpart

include $(GMAKERULES)
