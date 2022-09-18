#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/lib

OBJS= pow.o gcd.o msqrt.o mdiv.o mout.o mult.o madd.o util.o

ARFILE=libmp.a

install:
	$(INSTALL) -c libmp.a $(DESTROOT)/usr/lib/libmp.a
	$(RANLIB) $(DESTROOT)/usr/lib/libmp.a

pow.o:		pow.c
gcd.o:		gcd.c
msqrt.o:	msqrt.c
mdiv.o:		mdiv.c
mout.o:		mout.c
mult.o:		mult.c
madd.o:		madd.c
util.o:		util.c

include $(GMAKERULES)
