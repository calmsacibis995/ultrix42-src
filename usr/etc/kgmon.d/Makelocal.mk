# @(#)Makelocal.mk	4.1	(ULTRIX)	7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc/ $(DESTROOT)/etc

AOUT=	kgmon

OBJS=	kgmon.o

kgmon.o:	kgmon.c

install:
	$(INSTALL) -c -s kgmon ${DESTROOT}/usr/etc/kgmon
	$(RM) ${DESTROOT}/etc/kgmon
	$(LN) -s ../usr/etc/kgmon ${DESTROOT}/etc/kgmon

include $(GMAKERULES)
