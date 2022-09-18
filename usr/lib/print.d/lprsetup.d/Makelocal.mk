# @(#)Makelocal.mk	4.4      ULTRIX 	3/13/91

# Makefile for lprsetup.d
#
#
# AUTHOR:	Adrian Thoms
# DATE:		28th February 1989

include $(GMAKEVARS)

# uncomment next line for debugging version
#CDEFINES=-DLOCAL


CINCLUDES =	-I. -I.. -I../../h.d -I$(SRCROOT)/usr/include

SETUPDIR=usr/etc

EXDIR=usr/examples/print

DESTLIST=\
	$(DESTROOT)/etc \
	$(DESTROOT)/$(SETUPDIR) \
	$(DESTROOT)/usr \
	$(DESTROOT)/usr/examples \
	$(DESTROOT)/$(EXDIR)

LIBLP=../../lib.d/_$(MACHINE).b/liblp.a

AOUT=lprsetup

LOADLIBES=$(LIBLP)

CFILES= lprsetup.c misc.c printlib.c
HFILES= globals.h lprsetup.h

OBJS= lprsetup.o misc.o printlib.o safesyscalls.o

install:
	$(INSTALL) -c -s -m 755 lprsetup ${DESTROOT}/${SETUPDIR}
	$(INSTALL) -c -m 644 -o root ../printcap.examples $(DESTROOT)/$(EXDIR)/printcap

	rm -f ${DESTROOT}/etc/lprsetup
	ln -s ../usr/etc/lprsetup ${DESTROOT}/etc/lprsetup


lprsetup.o:	$(HFILES)
misc.o:		lprsetup.h

lprsetup.o: lprsetup.c
misc.o: misc.c
printlib.o: printlib.c

safesyscalls.o: safesyscalls.c


include $(GMAKERULES)












