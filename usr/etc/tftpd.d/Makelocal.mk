#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc

AOUT=	tftpd

OBJS=	tftpd.o tftpsubs.o

tftpd.o:	tftpd.c

tftpsubs.o:	../../../ucb/tftp.d/tftpsubs.c
	$(CCCMD) ../../../ucb/tftp.d/tftpsubs.c

AOUTS=	bootpd

bootpd:		bootpd.o
bootpd.o:	bootpd.c

install:
	$(INSTALL) -c -s tftpd $(DESTROOT)/usr/etc/tftpd
	$(INSTALL) -c -s bootpd $(DESTROOT)/usr/etc/bootpd

include $(GMAKERULES)
