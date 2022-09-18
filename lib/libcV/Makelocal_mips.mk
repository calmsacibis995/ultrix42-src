# @(#)Makelocal_mips.mk	4.1	(ULTRIX)	7/3/90

OBJSDIRS=../gen/_mips.b \
	../sys/_mips.b \
	../stdio/_mips.b \
	../_mips.d/sys/_mips.b 

all:	libcV.a

libcV.a: FRC
	@for i in $(OBJSDIRS); do \
		for j in $$i/*.o; do \
			$(RM) `basename $$j`; \
			$(LN) -s $$j `basename $$j`; \
		done; \
	done
	ar cr libcV.a *.o
	ranlib libcV.a

install:
	$(INSTALL) -c -m 644 libcV.a $(DESTROOT)/usr/lib/libcV.a
	ranlib $(DESTROOT)/usr/lib/libcV.a
