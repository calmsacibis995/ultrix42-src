#  @(#)Makelocal.mk	4.1	(ULTRIX)	7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/etc \
	$(DESTROOT)/usr/etc \
	$(DESTROOT)/usr/var/adm/ris \
	$(DESTROOT)/usr/diskless

LOADLIBES= /usr/lib/liblmf.a

AOUT=makpkt 
OBJS=makpkt.o

all:	$(AOUT)

install:	$(AOUT)
	$(INSTALL) -c -m 700 -o root -g system ../ris $(DESTROOT)/etc/ris
	$(INSTALL) -c -s -m 700 -o root -g system makpkt \
		$(DESTROOT)/usr/etc/makpkt
	$(INSTALL) -c -m 750 -o root -g system ../getnetif \
		$(DESTROOT)/usr/etc/getnetif
	$(LN) -s ../usr/etc/makpkt $(DESTROOT)/usr/diskless/makpkt

makpkt:		makpkt.o
makpkt.o:	makpkt.c

include $(GMAKERULES)
