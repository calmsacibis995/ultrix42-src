#
# @(#)Makelocal.mk	4.2	ULTRIX	8/13/90
#

include $(GMAKEVARS)

LOADLIBES=-ly

OBJS=m4.o m4ext.o m4macs.o m4y.o
AOUT=m4

m4.o: m4.c
m4ext.o: m4ext.c
m4macs.o: m4macs.c
m4y.o: m4y.y

pretools tools1 tools2: $(AOUT)
pretools tools1 tools2 install:
	$(INSTALL) -c -s $(AOUT) $(DESTROOT)/usr/bin

include $(GMAKERULES)
