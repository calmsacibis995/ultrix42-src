#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	quotaon

OBJS=	quotaon.o

quotaon.o:	quotaon.c

install:
	$(INSTALL) -c -s quotaon $(DESTROOT)/usr/etc/quotaon
	$(RM) $(DESTROOT)/etc/quotaon
	$(LN) -s ../usr/etc/quotaon $(DESTROOT)/etc/quotaon
	$(RM) $(DESTROOT)/etc/quotaoff
	$(LN) -s ../etc/quotaon $(DESTROOT)/etc/quotaoff

include $(GMAKERULES)
