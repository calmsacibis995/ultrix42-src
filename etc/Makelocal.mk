# @(#)Makelocal.mk	4.2      ULTRIX	3/7/91
#
include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/etc

SUBDIRS=dms.d elcsd.d eli.d htable.d krbpush.d mop.d namedb.d opser.d \
	ris.d termcap.d tzone.d uerf.d sec.d 

install:
	-$(INSTALL) -c ../mklost+found.sh $(DESTROOT)/etc/mklost+found
	-$(INSTALL) -c ../chk.sh $(DESTROOT)/etc/chk
	-$(INSTALL) -c -m 644 -o root ../acucap $(DESTROOT)/etc/acucap
	-$(INSTALL) -c -m 644 -o root /dev/null $(DESTROOT)/etc/dgateway
	-$(INSTALL) -c -m 644 -o root /dev/null $(DESTROOT)/etc/mtab
	-$(INSTALL) -c -m 644 -o root /dev/null $(DESTROOT)/etc/utmp
	-$(INSTALL) -c -m 644 -o root /dev/null $(DESTROOT)/etc/dumpdates

include $(GMAKERULES)
