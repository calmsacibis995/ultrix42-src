# @(#)Makelocal.mk	4.1	ULTRIX	7/17/90
#
include $(GMAKEVARS)

DESTLIST=${DESTROOT}/usr/bin

CDEFINES = -DVMUNIX -DTURN_OFF_MESSAGES

AOUT= troff
OBJS = ni.o nii.o n1.o n2.o n3.o n4.o n5.o t6.o n7.o n8.o n9.o t10.o tab3.o \
	hytab.o suftab.o

hytab.o: hytab.c
	$(CCCMD) -R ../hytab.c

suftab.o: suftab.c
	$(CCCMD) -R ../suftab.c

install:
	$(INSTALL) -c -s troff ${DESTROOT}/usr/bin/troff

ni.o:	ni.c
nii.o:	nii.c
n1.o:	n1.c
n2.o:	n2.c
n3.o:	n3.c
n4.o:	n4.c
n5.o:	n5.c
t6.o:	t6.c
n7.o:	n7.c
n8.o:	n8.c
n9.o:	n9.c
t10.o:	t10.c
tab3.o:	tab3.c

include $(GMAKERULES)
