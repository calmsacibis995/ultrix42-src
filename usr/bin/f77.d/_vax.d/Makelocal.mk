#  @(#)Makelocal.mk	4.1	ULTRIX  7/17/90

.SUFFIXES:
.SUFFIXES: .z

include $(GMAKEVARS)

.DEFAULT:
	($(CD) .. ;\
	$(MAKE) -$(MAKEFLAGS) $(MAKEDEFS) DESTROOT=$(DESTROOT) $@)
