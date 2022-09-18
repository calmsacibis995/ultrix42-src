include $(GMAKEVARS)
SUBDIRS=_$(MACHINE).d
tools2 install: $(SUBDIRS)
include $(GMAKERULES)
