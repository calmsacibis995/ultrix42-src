# @(#)Makelocal_vax.mk	4.1	ULTRIX	7/3/90

OBJSDIRS= ../gen/_vax.b \
	../stdio/_vax.b \
	../_vax.d/gen/_vax.b

all:	libcP.a libcPg.a libcP_p.a

libcP.a: FRC
	@for i in $(OBJSDIRS); do \
		for j in $$i/*.o; do \
			$(ECHO) "$(RM) `basename $$j`"; \
			$(RM) `basename $$j`; \
			$(ECHO) "$(LN) -s $$j `basename $$j`"; \
			$(LN) -s $$j `basename $$j`; \
		done; \
	done
	ar cr libcP.a `lorder *.o | tsort`
	ranlib libcP.a

libcPg.a: FRC
	@for i in $(OBJSDIRS); do \
		for j in $$i/gfloat/*.o; do \
			$(ECHO) "$(RM) gfloat/`basename $$j`"; \
			$(RM) gfloat/`basename $$j`; \
			$(ECHO) "$(LN) -s ../$$j gfloat/`basename $$j`"; \
			$(LN) -s ../$$j gfloat/`basename $$j`; \
		done; \
	done
	cd gfloat; ar cr ../libcPg.a `lorder *.o | tsort`
	ranlib libcPg.a

libcP_p.a: FRC
	@for i in $(OBJSDIRS); do \
		for j in $$i/profiled/*.o; do \
			$(ECHO) "$(RM) profiled/`basename $$j`"; \
			$(RM) profiled/`basename $$j`; \
			$(ECHO) "$(LN) -s ../$$j profiled/`basename $$j`"; \
			$(LN) -s ../$$j profiled/`basename $$j`; \
		done; \
	done
	cd profiled; ar cr ../libcP_p.a `lorder *.o | tsort`
	ranlib libcP_p.a

clean: cleangfloat cleanprofiled

cleangfloat:
	-$(RM) gfloat/*

cleanprofiled:
	-$(RM) profiled/*

install:
	$(INSTALL) -c -m 644 libcP.a $(DESTROOT)/usr/lib/libcP.a
	ranlib $(DESTROOT)/usr/lib/libcP.a
	$(INSTALL) -c -m 644 libcPg.a $(DESTROOT)/usr/lib/libcPg.a
	ranlib $(DESTROOT)/usr/lib/libcPg.a
	$(INSTALL) -c -m 644 libcP_p.a $(DESTROOT)/usr/lib/libcP_p.a
	ranlib $(DESTROOT)/usr/lib/libcP_p.a
