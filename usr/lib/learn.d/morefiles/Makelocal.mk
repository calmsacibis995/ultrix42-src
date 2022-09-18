#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/lib/learn/morefiles

install:
	$(CP) ../L* $(DESTROOT)/usr/lib/learn/morefiles

include $(GMAKERULES)
