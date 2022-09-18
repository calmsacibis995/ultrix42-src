#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/bin $(DESTROOT)/etc

AOUT=	reboot

OBJS=	reboot.o

reboot.o:	reboot.c

install:
	$(INSTALL) -c -s reboot $(DESTROOT)/bin/reboot
	$(RM) $(DESTROOT)/etc/reboot
	$(LN) -s ../bin/reboot $(DESTROOT)/etc/reboot

include $(GMAKERULES)
