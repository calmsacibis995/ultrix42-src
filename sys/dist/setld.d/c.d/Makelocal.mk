#	Makelocal.mk
#		sys/dist/setld.d/c.d Makefile
#
#  "@(#)Makelocal.mk	4.3 (ULTRIX) 11/9/90"
#
#	000	03-mar-1989	ccb
#	New.
#
#	001	11-may-1989	ccb
#		update installation support.
#		stripped binaries installed to /etc/stl
#
#	002	14-jun-1989	ccb
#		add /etc/stl/depord
#
#	003	24-jul-1989	ccb
#		add library dependencies
#		move ils destination to etc/stl
#		add var/adm/install as a destination
#		add lint target
#
#	004	17-oct-1989	ccb
#		add make rules for tclear
#
#	005	10-sep-1990	ccb
#		add rules for fsmount
#
#	006	06-nov-1990	ccb
#		Move etc/stl to /usr.

include $(GMAKEVARS)

LINT= lint
LINTFLAGS= -abh

# Define Directories
DIST= usr/sys/dist
STL= usr/etc/stl
LOGS= var/adm/install
HDRDIR= ../../h.d

# Define Libraries
STLLIB= ../../lib.d/_$(MACHINE).b/libsetld.a
LINTLIB= ../../lib.d/_$(MACHINE).b/llib-lsetld.ln

# Source Files (used to make lint)
SRCFILES= depord.c fitset.c frm.c fsmount.c fverify.c iff.c ils.c invcutter.c \
	tarsets.c tclear.c udelta.c udetect.c umerge.c usync.c

# Define Object Files - divided into 
DISTOBJS= invcutter.o tarsets.o
STLOBJS= depord.o fitset.o frm.o fsmount.o fverify.o iff.o ils.o tclear.o \
	udelta.o udetect.o umerge.o usync.o

# Define Binaries
DISTBINS= invcutter tarsets			
STLBINS= depord fitset frm fsmount fverify iff ils tclear udelta udetect \
	umerge usync


# Definitions Required for generic makefiles
DESTLIST= $(DESTROOT)/etc $(DESTROOT)/$(STL) $(DESTROOT)/$(DIST) \
	$(DESTROOT)/$(LOGS)
CFLAGS=-O $(DBX)
CINCLUDES= -I. -I.. -I$(SRCROOT)/usr/include -I$(HDRDIR)
LOADLIBES= $(STLLIB)
MYAOUTS= $(DISTBINS) $(STLBINS)

all:		$(MYAOUTS)

lint:		$(LINTLIB) $(SRCFILES)
	@for i in $(SRCFILES); \
	do \
		$(ECHO) "$(LINT) $(LINTFLAGS) $(CINCLUDES) ../$$i $(LINTLIB)"; \
		$(LINT) $(LINTFLAGS) $(CINCLUDES) ../$$i $(LINTLIB); \
	done

install:	$(MYAOUTS)
	# install DISTBINS
	@for i in $(DISTBINS); \
	do \
		$(ECHO) "$(INSTALL) -s -c -m 755 $$i $(DESTROOT)/$(DIST)/$$i"; \
		$(INSTALL) -s -c -m 755 $$i $(DESTROOT)/$(DIST)/$$i; \
	done
	@for i in $(STLBINS); \
	do \
		$(ECHO) "$(INSTALL) -c -s -m 755 $$i $(DESTROOT)/$(STL)/$$i"; \
		$(INSTALL) -c -s -m 755 $$i $(DESTROOT)/$(STL)/$$i; \
	done
	(cd $(DESTROOT)/etc; ln -s ../$(STL) stl)


# myaouts: this rules is a modification on the generic rule, it
#  permits re-link dependent upon library module changes

$(MYAOUTS):
	$(LDCMD) $@.o $(LOADLIBES)


# individual module dependencies the .o lines show dependencies
#  requiring re-compile, the non-.o lines show lines that require
#  a re-link. The library dependencies are constructed using cflow(1).

depord.o:	depord.c $(HDRDIR)/list.h $(HDRDIR)/setld.h
depord:		depord.o $(STLLIB)(Ctrl.o) $(STLLIB)(Name.o) \
			$(STLLIB)(list.o) $(STLLIB)(Prod.o)

fitset.o:	fitset.c $(HDRDIR)/setld.h
fitset:		fitset.o $(STLLIB)(inv.o)

frm.o:		frm.c $(HDRDIR)/setld.h
frm:		frm.o $(STLLIB)(getsum.o) $(STLLIB)(inv.o)

fsmount.o:	fsmount.c $(HDRDIR)/setld.h
frm:		fsmount.o $(STLLIB)(inv.o)

fverify.o:	fverify.c $(HDRDIR)/setld.h
fverify:	fverify.o $(STLLIB)(PermString.o) $(STLLIB)(inv.o) \
			$(STLLIB)(verify.o)

iff.o:		iff.c $(HDRDIR)/setld.h
iff:		iff.o $(STLLIB)(inv.o)

ils.o:		ils.c $(HDRDIR)/setld.h
ils:		ils.o  $(STLLIB)(getsum.o) $(STLLIB)(inv.o)

invcutter.o:	invcutter.c $(HDRDIR)/setld.h
invcutter:	invcutter.o $(STLLIB)(getsum.o) $(STLLIB)(inv.o) \
			$(STLLIB)(mi.o)

tclear.o:	tclear.c $(HDRDIR)/setld.h
tclear:		tclear.o $(STLLIB)(inv.o)

tarsets.o:	tarsets.c $(HDRDIR)/setld.h
tarsets:	tarsets.o

udelta.o:	udelta.c $(HDRDIR)/setld.h
udelta:		udelta.o

udetect.o:	udetect.c $(HDRDIR)/setld.h
udetect:	udetect.o $(STLLIB)(inv.o) $(STLLIB)(verify.o)

umerge.o:	umerge.c $(HDRDIR)/setld.h
umerge:		umerge.o $(STLLIB)(inv.o)

usync.o:	usync.c $(HDRDIR)/setld.h
usync:		usync.o

include $(GMAKERULES)

