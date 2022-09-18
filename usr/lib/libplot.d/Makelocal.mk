#  @(#)Makelocal.mk	4.1	ULTRIX	7/2/90

include $(GMAKEVARS)

SUBDIRS=tf77 plot t4013 t4014 t300 t300s t450 vt0 aed bitgraph dumb gigi \
	hp2648 hp7221 imagen grn lvp16

install: $(SUBDIRS)

include $(GMAKERULES)
