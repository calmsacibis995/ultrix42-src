#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90

include $(GMAKEVARS)

# Defines needed in diff.c
DIFF=	/bin/diff
DIFFH=	/usr/lib/diffh
PR=	/bin/pr
CDEFINES=	-DDIFF='"${DIFF}"' -DDIFFH='"${DIFFH}"' -DPR='"${PR}"'

AOUT=diff

OBJS=diff.o diffdir.o diffreg.o
diff.o:		diff.c diff.h
diffdir.o:	diffdir.c diff.h
diffreg.o:	diffreg.c diff.h

AOUTS=diffh

diffh:		diffh.o
diffh.o:	diffh.c

install:
	$(INSTALL) -c -s diff ${DESTROOT}/usr/bin/diff
	$(RM) ${DESTROOT}/bin/diff
	$(LN) -s ../usr/bin/diff ${DESTROOT}/bin/diff
	$(INSTALL) -c -s diffh ${DESTROOT}/usr/lib/diffh

include $(GMAKERULES)
