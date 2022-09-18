#  "@(#)Makelocal.mk	4.1 (ULTRIX) 7/2/90"

include $(GMAKEVARS)

TODIR= $(DESTROOT)/usr/sys/dist
DESTLIST= $(TODIR)

SUBDIRS= _$(MACHINE).d

install:
	$(INSTALL) -c -m 644 ../kitcap $(TODIR)/kitcap

include $(GMAKERULES)
