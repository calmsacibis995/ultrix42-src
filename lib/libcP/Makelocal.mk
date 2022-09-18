# @(#)Makelocal.mk	4.1	ULTRIX	7/3/90

include $(GMAKEVARS)

SUBDIRS=gen stdio _$(MACHINE).d

include $(GMAKERULES)

include ../Makelocal_$(MACHINE).mk
