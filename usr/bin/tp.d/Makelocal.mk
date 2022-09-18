# @(#)Makelocal.mk	4.1	(ULTRIX)	7/17/90

include $(GMAKEVARS)
AOUT=tp
OBJS  = tp0.o tp1.o tp2.o tp3.o
CINCLUDES= -I..
h = tp.h tp_defs.h

$(OBJS) :	$h

install :
	$(INSTALL) -c -s tp ${DESTROOT}/usr/bin/tp
	$(RM) ${DESTROOT}/bin/tp
	$(LN) -s ../usr/bin/tp ${DESTROOT}/bin/tp

tp0.o:	tp0.c
tp1.o:	tp1.c
tp2.o:	tp2.c
tp3.o:	tp3.c

include $(GMAKERULES)
