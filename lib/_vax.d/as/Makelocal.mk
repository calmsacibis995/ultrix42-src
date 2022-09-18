#  @(#)Makelocal.mk	4.2  ULTRIX  9/12/90

include $(GMAKEVARS)

HDRS = astoks.H astokfix.awk as.h asscan.h assyms.h asexpr.h 

OBJS =	asscan1.o asscan2.o asscan3.o asscan4.o bignum1.o bignum2.o \
	natof.o floattab.o asparse.o asexpr.o asmain.o assyms.o \
	asjxxx.o ascode.o aspseudo.o assizetab.o asio.o

CDEFINES=-DAS

AOUT= as

all:	astoks.h 

asscan1.o:	asscan1.c
asscan2.o:	asscan2.c
asscan3.o:	asscan3.c
asscan4.o:	asscan4.c
bignum1.o:	bignum1.c
bignum2.o:	bignum2.c
natof.o:	natof.c
floattab.o:	floattab.c
asparse.o:	asparse.c
asexpr.o:	asexpr.c
asmain.o:	asmain.c
assyms.o:	assyms.c
asjxxx.o:	asjxxx.c
ascode.o:	ascode.c
aspseudo.o:	aspseudo.c
assizetab.o:	assizetab.c
asio.o:		asio.c

.c.o:	astoks.h

astoks.h: astoks.H astokfix.awk
	$(AWK) -f ../astokfix.awk < ../astoks.H > astoks.h

aspseudo.o:	as.h astoks.h aspseudo.c instrs.h instrs.as
	$(CC) -c -R $(CDEFINES) -I. -I.. ../aspseudo.c

instrs.as: instrs
	($(ECHO) FLAVOR AS; $(CAT) ../instrs) \
		| $(AWK) -f ../instrs > instrs.as

install:
	$(INSTALL) -c -s as ${DESTROOT}/usr/bin/as
	$(RM) ${DESTROOT}/bin/as
	$(LN) -s ../usr/bin/as ${DESTROOT}/bin/as

include $(GMAKERULES)
