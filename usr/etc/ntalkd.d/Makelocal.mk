#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc

AOUT=	talkd

OBJS=	talkd.o announce.o process.o table.o print.o

talkd.o:	talkd.c
announce.o:	announce.c
process.o:	process.c
table.o:	table.c
print.o:	print.c

install:
	$(INSTALL) -c -s -m 755 talkd $(DESTROOT)/usr/etc/ntalkd

include $(GMAKERULES)
