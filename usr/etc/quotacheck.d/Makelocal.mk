#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	quotacheck

OBJS=	quotacheck.o

quotacheck.o:	quotacheck.c

install:
	$(INSTALL) -c -s quotacheck $(DESTROOT)/usr/etc/quotacheck
	$(RM) $(DESTROOT)/etc/quotacheck
	$(LN) -s ../usr/etc/quotacheck $(DESTROOT)/etc/quotacheck

include $(GMAKERULES)
