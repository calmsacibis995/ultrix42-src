#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc $(DESTROOT)/etc

AOUTS=	kern_loop kern_unloop

kern_loop:	kern_loop.o
kern_loop.o:	kern_loop.c

kern_unloop:	kern_unloop.o
kern_unloop.o:	kern_unloop.c

install:
	$(INSTALL) -c -s  kern_loop $(DESTROOT)/usr/etc/kern_loop
	$(RM) $(DESTROOT)/etc/kern_loop
	$(LN) -s ../usr/etc/kern_loop $(DESTROOT)/etc/kern_loop
	$(INSTALL) -c -s  kern_unloop $(DESTROOT)/usr/etc/kern_unloop
	$(RM) $(DESTROOT)/etc/kern_unloop
	$(LN) -s ../usr/etc/kern_unloop $(DESTROOT)/etc/kern_unloop

include $(GMAKERULES)
