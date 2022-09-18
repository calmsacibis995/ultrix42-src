#  @(#)Makelocal.mk	4.1  ULTRIX  7/3/90

include $(GMAKEVARS)

COMPAT=		compat-4.1 compat-sys5

SUBDIRS=gen inet net rpc rpcsvc stdio ${COMPAT} _$(MACHINE).d

include $(GMAKERULES)

include ../Makelocal_$(MACHINE).mk
