# @(#)Makelocal.mk	4.1	(ULTRIX)	7/17/90
include $(GMAKEVARS)
AOUT=mail
OBJS=mail.o

mail.o:	mail.c

install:
	install -c -s -m 6755 -g kmem mail ${DESTROOT}/usr/bin/mail
	$(RM) ${DESTROOT}/bin/mail
	ln -s ../usr/bin/mail ${DESTROOT}/bin/mail

include $(GMAKERULES)
