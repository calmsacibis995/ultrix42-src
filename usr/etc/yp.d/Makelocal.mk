#  @(#)Makelocal.mk	4.1	ULTRIX	7/2/90

include $(GMAKEVARS)

DESTLIST=${DESTROOT}/usr/var/yp ${DESTROOT}/etc

SUBDIRS=revnetgroup mknetid

LOADLIBES = -ldbm

AOUTS=	makedbm ypxfr yppush ypset yppoll stdhosts stdethers mkalias

SCRIPT= ypinit ypxfr_1perday ypxfr_2perday ypxfr_1perhour

makedbm:	makedbm.o
makedbm.o:	makedbm.c

ypxfr:		ypxfr.o
ypxfr.o:	ypxfr.c

yppush:		yppush.o
yppush.o:	yppush.c

ypset:		ypset.o
ypset.o:	ypset.c

yppoll:		yppoll.o
yppoll.o:	yppoll.c

stdhosts:	stdhosts.o
stdhosts.o:	stdhosts.c

stdethers:	stdethers.o
stdethers.o:	stdethers.c

mkalias:	mkalias.o
mkalias.o:	mkalias.c


install:
	$(INSTALL) -s -c makedbm $(DESTROOT)/usr/var/yp/makedbm
	$(INSTALL) -s -c yppush $(DESTROOT)/usr/var/yp/yppush
	$(INSTALL) -s -c ypset $(DESTROOT)/usr/var/yp/ypset
	$(INSTALL) -s -c ypxfr $(DESTROOT)/usr/var/yp/ypxfr
	$(INSTALL) -s -c yppoll $(DESTROOT)/usr/var/yp/yppoll
	$(INSTALL) -s -c stdhosts $(DESTROOT)/usr/var/yp/stdhosts
	$(INSTALL) -s -c mkalias $(DESTROOT)/usr/var/yp/mkalias
	@for i in ${SCRIPT}; do \
		$(ECHO) "$(INSTALL) -c ../$$i.sh ${DESTROOT}/usr/var/yp/$$i"; \
		$(INSTALL) -c ../$$i.sh ${DESTROOT}/usr/var/yp/$$i; \
	done
	$(INSTALL) -c ../make.script ${DESTROOT}/usr/var/yp/Makefile
	$(RM) ${DESTROOT}/etc/yp
	$(LN) -s ../var/yp ${DESTROOT}/etc/yp

include $(GMAKERULES)
