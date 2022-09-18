include $(GMAKEVARS)
SUBDIRS=_$(MACHINE).d
pretools tools1 tools2 install: $(SUBDIRS)
include $(GMAKERULES)
