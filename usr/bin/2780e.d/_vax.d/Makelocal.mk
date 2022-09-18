#  @(#)Makelocal.mk	4.1  ULTRIX  7/17/90
# Make template for Makelocal.mk
# Should be used to make sources within a directory and subdirectories

# First thing is to include the general variables for the machine
# type.
include $(GMAKEVARS)

LINCLUDES=-I../../../../etc/2780d.d/_$(MACHINE).d \
	-I.. -I$(SRCROOT)/usr/include
CFLAGS=-O
LDFLAGS=${CFLAGS} -s

all: 2780e 3780e

2780e:	2780e.c
	$(CC) $(LDFLAGS) $(LINCLUDES) ../$? -o $@
3780e:	2780e.c
	$(CC) $(LDFLAGS) $(LINCLUDES) -Dd3780 ../$? -o $@

install: 
	${INSTALL} -c -s -m 4755 2780e ${DESTROOT}/usr/bin/2780e
	${INSTALL} -c -s -m 4755 3780e ${DESTROOT}/usr/bin/3780e

# All vars are set above.
# Include rules file that will perform operations based on those vars.
include $(GMAKERULES)
