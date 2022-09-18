#  @(#)Makelocal.mk	4.2  ULTRIX  8/8/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/lib $(DESTROOT)/etc

SUBDIRS=adb.d cda.d ddif.d find.d font.d fontinfo.d intln.d learn.d \
	lib2648.d libaud.d libauth.d libcapsar.d libcurses.d libcursesX.d \
	libdbm.d libdnet.d liberrlog.d libg.d libi.d libkrb.d liblmf.d libln.d \
	libmalloc.d libmp.d libmV.d libnm.d librpc.d libpc.d \
	libplot.d libsnmp.d libtermlib.d liby.d print.d me.d ms.d obj.d ps.d \
	sendmail.d tmac.d upars.d uucp.d vfont.d whatis.d libxti.d liboldbm.d 

AOUTS=	atrun getNAME makekey

atrun:		atrun.o
getNAME:	getNAME.o
makekey:	makekey.o

atrun.o:	atrun.c
getNAME.o:	getNAME.c
makekey.o:	makekey.c

install:
	$(INSTALL) -s -c atrun $(DESTROOT)/usr/lib/atrun
	$(INSTALL) -s -c getNAME $(DESTROOT)/usr/lib/getNAME
	$(INSTALL) -s -c makekey $(DESTROOT)/usr/lib/makekey
	$(INSTALL) -c ../makewhatis.sh $(DESTROOT)/usr/lib/makewhatis
	$(INSTALL) -c ../crontab $(DESTROOT)/etc/crontab
	$(RM) $(DESTROOT)/usr/lib/crontab
	$(LN) -s ../../etc/crontab $(DESTROOT)/usr/lib/crontab
	$(INSTALL) -c ../lib.b $(DESTROOT)/usr/lib/lib.b
	$(INSTALL) -c -m 644 ../eign $(DESTROOT)/usr/lib/eign
	$(INSTALL) -c -m 644 ../whatis.d/_$(MACHINE).d/whatis \
		$(DESTROOT)/usr/lib/whatis

include $(GMAKERULES)
