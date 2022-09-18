#  @(#)Makelocal.mk	4.1	ULTRIX	7/2/90

include $(GMAKEVARS)

DESTLIST=${DESTROOT}/etc/zoneinfo/sources ${DESTROOT}/etc/zoneinfo ${DESTROOT}/etc

DATA=	../africa ../asia ../australasia ../etcetera ../europe ../factory \
../northamerica ../pacificnew ../southamerica ../systemv

AOUTS=		zdump zic

ialloc.o:	ialloc.c
scheck.o:	scheck.c
zic.o:		zic.c
zdump.o:	zdump.c
zic:		zic.o ialloc.o scheck.o
zdump:		zdump.o ialloc.o

install:
	./zic -d ${DESTROOT}/etc/zoneinfo $(DATA)
	for i in $(DATA); do \
		$(INSTALL) -c -m 644 $$i \
			$(DESTROOT)/etc/zoneinfo/sources/`basename $$i`; \
	done
	$(INSTALL) -s -c zic $(DESTROOT)/etc/zic
	$(INSTALL) -s -c zdump $(DESTROOT)/etc/zdump

include $(GMAKERULES)
