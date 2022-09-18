# @(#)Makelocal_mips.mk	4.1	ULTRIX	7/3/90

OBJSDIRS=../gen/_mips.b \
	../stdio/_mips.b \
	../_mips.d/gen/_mips.b

all:	libcP.a

libcP.a: FRC
	@for i in $(OBJSDIRS); do \
		for j in $$i/*.o; do \
			$(RM) `basename $$j`; \
			$(LN) -s $$j `basename $$j`; \
		done; \
	done
	ar cr libcP.a *.o
	ranlib libcP.a

install:
	$(INSTALL) -c -m 644 libcP.a $(DESTROOT)/usr/lib/libcP.a
	ranlib $(DESTROOT)/usr/lib/libcP.a
