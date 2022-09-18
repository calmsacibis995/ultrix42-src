#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

include $(GMAKEVARS)

DESTLIST=$(DESTROOT)/usr/etc $(DESTROOT)/etc

CFLAGS= -DSNMP -g

AOUT=	routed

OBJS=	af.o if.o input.o main.o output.o startup.o tables.o timer.o \
	trace.o inet.o ext.o snmp.o

af.o:		af.c
if.o:		if.c
input.o:	input.c
main.o:		main.c
output.o:	output.c
startup.o:	startup.c
tables.o:	tables.c
timer.o:	timer.c
trace.o:	trace.c
inet.o:		inet.c
ext.o:		ext.c
snmp.o:		snmp.c

install:
	$(INSTALL) -c -s routed $(DESTROOT)/usr/etc/routed
	$(RM) $(DESTROOT)/etc/routed
	$(LN) -s ../usr/etc/routed $(DESTROOT)/etc/routed

include $(GMAKERULES)
