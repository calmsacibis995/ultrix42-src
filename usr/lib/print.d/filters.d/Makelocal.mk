# @(#)Makelocal.mk	4.3      ULTRIX 	11/15/90

# File:		Makelocal.mk for filters.d
# Author:	Adrian Thoms (thoms@wessex)
#
# Modification History:
#
# 26-Oct-90 - Adrian Thoms (thoms@wessex)
#	Increase optimiser limit for lj250of.c
#
# 27-Sep-90 - Adrian Thoms (thoms@wessex)
#	File guesser for lj250of lcg01of and decuniversal_of
#	is now linked in from a library module

include $(GMAKEVARS)

LJ250OF_CFLAGS=-O -Olimit 3260

DESTDIR=$(DESTROOT)/usr/lib/lpdfilters

AOUTS=	lpf necf rvsort rvcat vsort vcat vpf vpsf vdmp vpltdmp \
	vfontinfo vwidth vfw fcvt rotate rotprt ln01of ln01pp \
	lcg01of lj250of lqf 


AOUTSMISC= vplotf ln03rof
AOUTMERGED= decuniversal_of

# Alternate names of ln03rof filter
LN03ROF_ISOLATIN1= ln03rof_isolatin1
LN03ROF_DECMCS= ln03rof_decmcs

# Alternate names of decuniversal_of filter
LN03OF= ln03of
LG31OF= lg31of
LG02OF= lg02of
LA75OF= la75of

FILTERLIB= filter

LIBINTLN= -li
LIBFILTER= -l${FILTERLIB}

FILTERLIBDIR=../../libfilter.d/_${MACHINE}.b

LOADLIBES= ${LIBINTLN} ${LIBFILTER}

CINCLUDES=-I. -I.. -I../../libfilter.d
LDFLAGS=${CFLAGS} ${CINCLUDES} -L${FILTERLIBDIR}

all: $(AOUTSMISC) $(AOUTMERGED)

lpf:		lpf.o
necf:		necf.o
rvsort:		rvsort.o
rvcat:		rvcat.o
vsort:		vsort.o
vcat:		vcat.o
vpf:		vpf.o
vpsf:		vpsf.o
vdmp:		vdmp.o
vpltdmp:	vpltdmp.o
vfontinfo:	vfontinfo.o
vwidth:		vwidth.o
vfw:		vfw.o
fcvt:		fcvt.o
rotate:		rotate.o
rotprt:		rotprt.o
ln01of:		ln01of.o
ln01pp:		ln01pp.o
lcg01of:	lcg01of.o
lj250of:	lj250of.o
lqf:		lqf.o

chrtab.o:	chrtab.c

lpf.o:		lpf.c
necf.o:		necf.c
rvsort.o:	rvsort.c
rvcat.o:	rvcat.c
vsort.o:	vsort.c
vcat.o:		vcat.c
vpf.o:		vpf.c
vpsf.o:		vpsf.c
vdmp.o:		vdmp.c
vpltdmp.o:	vpltdmp.c
vfontinfo.o:	vfontinfo.c
vwidth.o:	vwidth.c
vfw.o:		vfw.c
fcvt.o:		fcvt.c
rotate.o:	rotate.c
rotprt.o:	rotprt.c
ln01of.o:	ln01of.c
ln01pp.o:	ln01pp.c
lcg01of.o:	lcg01of.c
lj250of.o:	lj250of.c
	$(CC) -c $(CDEBUG) $(LJ250OF_CFLAGS) $(CDEFINES) $(CINCLUDES) ../$<

lqf.o:		lqf.c


vplotf: vplotf.c chrtab.o
	$(CC) -o vplotf $(CFLAGS) ../$@.c chrtab.o

ln03rof: ln03rof.c
	$(LDCMD) -DLN03ROF_DECMCS=\"${LN03ROF_DECMCS}\" \
	-DLN03ROF_ISOLATIN1=\"${LN03ROF_ISOLATIN1}\" ../$@.c $(LOADLIBES)

decuniversal_of:	decuniversal_of.o
	$(LDCMD) decuniversal_of.o $(LOADLIBES)

decuniversal_of.o: decuniversal_of.c
	$(CCCMD) \
	-DLN03OF=\"${LN03OF}\" \
	-DLG31OF=\"${LG31OF}\" \
	-DLG02OF=\"${LG02OF}\" \
	-DLA75OF=\"${LA75OF}\" \
	../decuniversal_of.c

install: install.dir install.ln03r install.merge install.misc

install.dir:
	-@if [ ! -d $(DESTDIR) ]; then \
		$(ECHO) "$(MKDIR) $(DESTDIR)"; \
		$(MKDIR) $(DESTDIR); \
		$(ECHO) "$(CHMOD) 755 $(DESTDIR)"; \
		$(CHMOD) 755 $(DESTDIR); \
		$(ECHO) "$(CHOWN) root $(DESTDIR)"; \
		$(CHOWN) root $(DESTDIR); \
		$(ECHO) "$(CHGRP) system $(DESTDIR)"; \
		$(CHGRP) system $(DESTDIR); \
	else \
		true; \
	fi

install.misc:
	@for i in $(AOUTS); do \
		$(ECHO) "$(INSTALL) -c -s $$i $(DESTDIR)/$$i"; \
		$(INSTALL) -c -s $$i $(DESTDIR)/$$i; \
	done
	@for i in $(AOUTSMISC); do \
		$(ECHO) "$(INSTALL) -c -s $$i $(DESTDIR)/$$i"; \
		$(INSTALL) -c -s $$i $(DESTDIR)/$$i; \
	done
	$(RM) $(DESTROOT)/usr/lib/rvsort
	$(LN) -s  lpdfilters/rvsort $(DESTROOT)/usr/lib/rvsort
	$(RM) $(DESTDIR)/vpfW
	$(LN) $(DESTDIR)/vpf $(DESTDIR)/vpfW
	$(RM) $(DESTDIR)/vpsfW
	$(LN) $(DESTDIR)/vpsf $(DESTDIR)/vpsfW
	$(INSTALL) -c ../lcg01sw.dat $(DESTDIR)/lcg01sw.dat
	$(INSTALL) -c ../xf.sh $(DESTDIR)/xf


install.ln03r: install.misc
	$(RM) $(DESTDIR)/${LN03ROF_ISOLATIN1}
	$(RM) $(DESTDIR)/${LN03ROF_DECMCS}
	$(LN) $(DESTDIR)/ln03rof $(DESTDIR)/${LN03ROF_ISOLATIN1}
	$(LN) $(DESTDIR)/ln03rof $(DESTDIR)/${LN03ROF_DECMCS}

install.merge:
	$(RM) $(DESTDIR)/${LG31OF}
	$(RM) $(DESTDIR)/${LG02OF}
	$(RM) $(DESTDIR)/${LA75OF}
	$(INSTALL) -c -s decuniversal_of $(DESTDIR)/${LN03OF}
	$(LN) $(DESTDIR)/${LN03OF} $(DESTDIR)/${LG31OF}
	$(LN) $(DESTDIR)/${LN03OF} $(DESTDIR)/${LG02OF}
	$(LN) $(DESTDIR)/${LN03OF} $(DESTDIR)/${LA75OF}


include $(GMAKERULES)
