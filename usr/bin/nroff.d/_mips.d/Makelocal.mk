#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

DESTLIST=${DESTROOT}/usr/bin

SUBDIRS=term_$(MACHINE).d

CDEFINES = -DNROFF -DVMUNIX -DTURN_OFF_MESSAGES
CINCLUDES=-I. -I.. -I$(SRCROOT)/usr/include -I../../../troff.d/_$(MACHINE).d

AOUT=	nroff

TOBJS=	ni.o nii.o n1.o n2.o n3.o n4.o n5.o n7.o n8.o n9.o

SOBJS=	n6.o n10.o hytab.o ntab.o suftab.o

OBJS=	$(TOBJS) $(SOBJS)

$(TOBJS):
	$(CCCMD) $?

ni.o:	../../../troff.d/_$(MACHINE).d/ni.c
nii.o:	../../../troff.d/_$(MACHINE).d/nii.c
n1.o:	../../../troff.d/_$(MACHINE).d/n1.c
n2.o:	../../../troff.d/_$(MACHINE).d/n2.c
n3.o:	../../../troff.d/_$(MACHINE).d/n3.c
n4.o:	../../../troff.d/_$(MACHINE).d/n4.c
n5.o:	../../../troff.d/_$(MACHINE).d/n5.c
n7.o:	../../../troff.d/_$(MACHINE).d/n7.c
n8.o:	../../../troff.d/_$(MACHINE).d/n8.c
n9.o:	../../../troff.d/_$(MACHINE).d/n9.c

n6.o:	n6.c
n10.o:	n10.c

hytab.o:	../../../troff.d/_$(MACHINE).d/hytab.c
	$(CCCMD) -R $?

ntab.o:		../../../troff.d/_$(MACHINE).d/ntab.c
	$(CCCMD) -R $?

suftab.o:	../../../troff.d/_$(MACHINE).d/suftab.c
	$(CCCMD) -R $?

install:
	$(INSTALL) -c -s nroff ${DESTROOT}/usr/bin/nroff

include $(GMAKERULES)
