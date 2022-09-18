OBJSDIRS= ../gen/_vax.b \
	../sys/_vax.b \
	../stdio/_vax.b \
	../_vax.d/sys/_vax.b

all:	libcV.a libcVg.a libcV_p.a

libcV.a: FRC
	@for i in $(OBJSDIRS); do \
		for j in $$i/*.o; do \
			$(ECHO) "$(RM) `basename $$j`"; \
			$(RM) `basename $$j`; \
			$(ECHO) "$(LN) -s $$j `basename $$j`"; \
			$(LN) -s $$j `basename $$j`; \
		done; \
	done
	ar cr libcV.a `lorder *.o | tsort`
	ranlib libcV.a

libcVg.a: FRC
	@for i in $(OBJSDIRS); do \
		for j in $$i/gfloat/*.o; do \
			$(ECHO) "$(RM) gfloat/`basename $$j`"; \
			$(RM) gfloat/`basename $$j`; \
			$(ECHO) "$(LN) -s ../$$j gfloat/`basename $$j`"; \
			$(LN) -s ../$$j gfloat/`basename $$j`; \
		done; \
	done
	cd gfloat; ar cr ../libcVg.a `lorder *.o | tsort`
	ranlib libcVg.a

libcV_p.a: FRC
	@for i in $(OBJSDIRS); do \
		for j in $$i/profiled/*.o; do \
			$(ECHO) "$(RM) profiled/`basename $$j`"; \
			$(RM) profiled/`basename $$j`; \
			$(ECHO) "$(LN) -s ../$$j profiled/`basename $$j`"; \
			$(LN) -s ../$$j profiled/`basename $$j`; \
		done; \
	done
	cd profiled; ar cr ../libcV_p.a `lorder *.o | tsort`
	ranlib libcV_p.a

clean: cleangfloat cleanprofiled

cleangfloat:
	-$(RM) gfloat/*

cleanprofiled:
	-$(RM) profiled/*

install:
	$(INSTALL) -c -m 644 libcV.a $(DESTROOT)/usr/lib/libcV.a
	ranlib $(DESTROOT)/usr/lib/libcV.a
	$(INSTALL) -c -m 644 libcVg.a $(DESTROOT)/usr/lib/libcVg.a
	ranlib $(DESTROOT)/usr/lib/libcVg.a
	$(INSTALL) -c -m 644 libcV_p.a $(DESTROOT)/usr/lib/libcV_p.a
	ranlib $(DESTROOT)/usr/lib/libcV_p.a
