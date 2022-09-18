# @(#)Makelocal_vax.mk	4.1  ULTRIX  7/3/90

OBJSDIRS=../net/_vax.b \
	../gen/_vax.b \
	../stdio/_vax.b \
	../compat-4.1/_vax.b \
	../compat-sys5/_vax.b \
	../inet/_vax.b \
	../rpc/_vax.b \
	../rpcsvc/_vax.b \
	../_vax.d/stdio/_vax.b \
	../_vax.d/sys/_vax.b \
	../_vax.d/gen/_vax.b \
	../_vax.d/compat-4.1/_vax.b \
	../_vax.d/net/_vax.b

KRBDIRS=../net/_vaxkrb.b

all:	libc.a libcg.a libc_p.a libckrb.a

libc.a: FRC
	@for i in $(OBJSDIRS); do \
		for j in $$i/*.o; do \
			$(ECHO) "$(RM) `basename $$j`"; \
			$(RM) `basename $$j`; \
			$(ECHO) "$(LN) -s $$j `basename $$j`"; \
			$(LN) -s $$j `basename $$j`; \
		done; \
	done
	ar cr libc.a `lorder *.o | tsort`
	ar m libc.a exit.o cleanup.o _exit.o cerror.o

libcg.a: FRC
	@for i in $(OBJSDIRS); do \
		for j in $$i/gfloat/*.o; do \
			$(ECHO) "$(RM) gfloat/`basename $$j`"; \
			$(RM) gfloat/`basename $$j`; \
			$(ECHO) "$(LN) -s ../$$j gfloat/`basename $$j`"; \
			$(LN) -s ../$$j gfloat/`basename $$j`; \
		done; \
	done
	cd gfloat; ar cr ../libcg.a `lorder *.o | tsort`
	ar m libcg.a exit.o cleanup.o _exit.o cerror.o

libc_p.a: FRC
	@for i in $(OBJSDIRS); do \
		for j in $$i/profiled/*.o; do \
			$(ECHO) "$(RM) profiled/`basename $$j`"; \
			$(RM) profiled/`basename $$j`; \
			$(ECHO) "$(LN) -s ../$$j profiled/`basename $$j`"; \
			$(LN) -s ../$$j profiled/`basename $$j`; \
		done; \
	done
	cd profiled; ar cr ../libc_p.a `lorder *.o | tsort`
	ar m libc_p.a exit.o cleanup.o _exit.o cerror.o

libckrb.a: FRC
	@for i in $(KRBDIRS); do \
		for j in $$i/*.o; do \
			$(ECHO) "$(RM) `basename $$j`"; \
			$(RM) `basename $$j`; \
			$(ECHO) "$(LN) -s $$j `basename $$j`"; \
			$(LN) -s $$j `basename $$j`; \
		done; \
	done
	ar cr libckrb.a `lorder *.o | tsort`
	ar m libckrb.a exit.o cleanup.o _exit.o cerror.o


clean: cleangfloat cleanprofiled

cleangfloat:
	@for files in `ls gfloat` ; do \
		$(RM) -f gfloat/$$files; \
	done

cleanprofiled:
	@for files in `ls profiled` ; do \
		$(RM) -f profiled/$$files; \
	done

install:
	${INSTALL} -c -m 644 libc.a ${DESTROOT}/usr/lib/libc.a
	ranlib ${DESTROOT}/usr/lib/libc.a
	${INSTALL} -c -m 644 libcg.a ${DESTROOT}/usr/lib/libcg.a
	ranlib ${DESTROOT}/usr/lib/libcg.a
	${INSTALL} -c -m 644 libc_p.a ${DESTROOT}/usr/lib/libc_p.a
	ranlib ${DESTROOT}/usr/lib/libc_p.a
	${INSTALL} -c -m 644 libckrb.a ${DESTROOT}/usr/lib/libckrb.a
	ranlib ${DESTROOT}/usr/lib/libckrb.a
