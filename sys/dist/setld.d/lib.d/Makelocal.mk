#	Makelocal.mk
#		sys/dist/setld.d/c.d Makefile
#
#  "@(#)Makelocal.mk	4.1 (ULTRIX) 7/2/90"
#
#	000	03-mar-1989	ccb
#	New.
#
#	001	01-may-1989	ccb
#		add PermString.o to libsetld.a
#
#	002	19-jun-1989	ccb
#		clean up module interdependencies to enhance the ability
#			to build correctly during development
#
#	003	24-jul-1989	ccb
#		add lint target for "make lint", creates and installs
#			llib-lsetld.ln for use linting the utility suite

include $(GMAKEVARS)

LINT= lint
LINTFLAGS= -abh

DESTLIB= $(DESTROOT)/usr/lib
DESTLINT= $(DESTLIB)/lint
HDRDIR= ../../h.d

SRCFILES= ../Assign.c ../Code.c ../Ctrl.c ../Date.c ../Deps.c ../Flags.c \
	../Name.c ../Path.c ../PermString.c ../Prod.c ../String.c \
	../emalloc.c ../getsum.c ../inv.c ../list.c ../mi.c ../verify.c

OBJFILES= Assign.o Code.o Ctrl.o Date.o Deps.o Flags.o \
	Name.o Path.o PermString.o \
	Prod.o String.o emalloc.o getsum.o inv.o list.o mi.o verify.o

LINTLIB= llib-lsetld.ln


DESTLIST= $(DESTLIB) $(DESTLINT)
CINCLUDES= -I. -I.. -I$(SRCROOT)/usr/include -I$(HDRDIR)
ARFILE= libsetld.a
OBJS= $(OBJFILES)

all:		$(ARFILE)

lint:		$(LINTLIB)

install:	$(ARFILE) $(LINTLIB)
	# install ARFILE
	$(ECHO) "$(INSTALL) -c -m 644 $$i $(DESTLIB)/$$i"
	$(INSTALL) -c -m 644 $(ARFILE) $(DESTLIB)/$(ARFILE)
	$(INSTALL) -c -m 644 $(LINTLIB) $(DESTLINT)/$(LINTLIB)

$(LINTLIB):	$(HDRDIR)/setld.h $(HDRDIR)/list.h $(SRCFILES)
	$(ECHO) "$(LINT) $(LINTFLAGS) -Csetld $(CINCLUDES) $(SRCFILES)"
	$(LINT) $(LINTFLAGS) -Csetld $(CINCLUDES) $(SRCFILES)


# Individual module dependencies. These are written so that modules in
#  the library are rebuilt if any of the routines called from within
#  them are rebuilt. This is done to force a relink of the correct programs
#  in ../c.d/Makelocal.mk. The programs there have dependency rules which
#  are written based on direct call knowledge only. The rules here allow
#  information about who calls who in the library to be hidden from the
#  application programs but still cause the application makefiles to do
#  the right thing.
#
# The module dependencies were derived by manually inspecting the source
#  for header file dependencies and by using cflow(1) to determine calling
#  relationships.

Assign.o:	Assign.c $(HDRDIR)/setld.h

Code.o:		Code.c $(HDRDIR)/setld.h

Ctrl.o:		Ctrl.c $(HDRDIR)/list.h $(HDRDIR)/setld.h Assign.c Deps.c \
			Flags.c list.c Name.c Path.c String.c

Date.o:		Date.c $(HDRDIR)/setld.h

Deps.o:		Deps.c $(HDRDIR)/list.h $(HDRDIR)/setld.h list.c Name.c \
			String.c

Flags.o:	Flags.c $(HDRDIR)/setld.h

Name.o:		Name.c $(HDRDIR)/setld.h

Path.o:		Path.c $(HDRDIR)/setld.h

PermString.o:	PermString.c

Prod.o:		Prod.c $(HDRDIR)/list.h $(HDRDIR)/setld.h Ctrl.c Code.c \
			Name.c Prod.c list.c

String.o:	String.c $(HDRDIR)/setld.h

emalloc.o:	emalloc.c

getsum.o:	getsum.c

inv.o:		inv.c $(HDRDIR)/list.h $(HDRDIR)/setld.h Code.c list.c

list.o:		list.c $(HDRDIR)/list.h

mi.o:		mi.c $(HDRDIR)/setld.h

verify.o:	verify.c $(HDRDIR)/setld.h getsum.c inv.c


include $(GMAKERULES)

