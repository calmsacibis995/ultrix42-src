# @(#)Makelocal_mips.mk	4.1   (ULTRIX)        7/3/90

OBJSDIRS=../net/_mips.b \
	../gen/_mips.b \
	../stdio/_mips.b \
	../compat-4.1/_mips.b \
	../compat-sys5/_mips.b \
	../inet/_mips.b \
	../rpc/_mips.b \
	../rpcsvc/_mips.b \
	../_mips.d/gen/_mips.b \
	../_mips.d/net/_mips.b \
	../_mips.d/stdio/_mips.b \
	../_mips.d/sys/_mips.b \
	../_mips.d/compat-sys5/_mips.b \
	../_mips.d/mld/_mips.b

KRBDIRS=../net/_mipskrb.b

all:	libc.a libc_G0.a libckrb.a

libc.a: FRC
	@for i in $(OBJSDIRS); do \
		for j in $$i/*.o; do \
			$(ECHO) "$(RM) `basename $$j`"; \
			$(RM) `basename $$j`; \
			$(ECHO) "$(LN) -s $$j `basename $$j`"; \
			$(LN) -s $$j `basename $$j`; \
		done; \
	done
	ar cr libc.a *.o
	ar m libc.a exit.o cleanup.o _exit.o cerror.o

libc_G0.a: FRC
	@for i in $(OBJSDIRS); do \
		for j in $$i/G0/*.o; do \
			$(ECHO) "$(RM) G0/`basename $$j`"; \
			$(RM) G0/`basename $$j`; \
			$(ECHO) "$(LN) -s ../$$j G0/`basename $$j`"; \
			$(LN) -s ../$$j G0/`basename $$j`; \
		done; \
	done
	cd G0; ar cr ../libc_G0.a *.o
	ar m libc_G0.a exit.o cleanup.o _exit.o cerror.o

libckrb.a: FRC
	@for i in $(KRBDIRS); do \
		for j in $$i/*.o; do \
			$(ECHO) "$(RM) `basename $$j`"; \
			$(RM) `basename $$j`; \
			$(ECHO) "$(LN) -s $$j `basename $$j`"; \
			$(LN) -s $$j `basename $$j`; \
		done; \
	done
	ar cr libckrb.a *.o
	ar m libckrb.a exit.o cleanup.o _exit.o cerror.o

install:
	$(INSTALL) -c -m 644 libc.a $(DESTROOT)/usr/lib/libc.a
	ranlib $(DESTROOT)/usr/lib/libc.a
	$(INSTALL) -c -m 644 libc_G0.a $(DESTROOT)/usr/lib/libc_G0.a
	ranlib $(DESTROOT)/usr/lib/libc_G0.a
	${INSTALL} -c -m 644 libckrb.a ${DESTROOT}/usr/lib/libckrb.a
	ranlib ${DESTROOT}/usr/lib/libckrb.a

clean: cleanG0

cleanG0:
	@for files in `ls G0` ; do \
		$(RM) -f G0/$$files; \
	done
