include $(GMAKEVARS)
#  @(#)Makelocal.mk	4.1  ULTRIX  7/2/90

LIB = ${DESTROOT}/usr/lib
SRCDIR = ${DESTROOT}/usr/ucb
#PASDIR = ${DESTROOT}/usr/src/ucb/pascal
PASDIR = ${SRCROOT}/usr/ucb/pascal

INSTALL= install
LD= /bin/ld
RM = /bin/rm -f
RANLIB= ranlib
AR= /bin/ar

CMDS =	ACTFILE.c ADDT.c ARGV.c ASRT.c ASRTS.c ATAN.c BUFF.c CARD.c \
	CASERNG.c CHR.c CLCK.c COS.c CTTOT.c DATE.c DEFNAME.c \
	DFDISPOSE.c DISPOSE.c EXCEPT.c EXP.c EXPO.c FCALL.c FLUSH.c \
	FNIL.c FRTN.c FSAV.c GET.c HALT.c IN.c INCT.c LINO.c \
	LLIMIT.c LN.c MAX.c MULT.c NAM.c NEW.c NIL.c PACK.c \
	PAGE.c PRED.c PUT.c RANDOM.c RANG4.c READ4.c READ8.c READC.c \
	READE.c READLN.c RELEQ.c RELNE.c RELSGE.c RELSGT.c RELSLE.c \
	RELSLT.c RELTGE.c RELTGT.c RELTLE.c RELTLT.c REMOVE.c RESET.c \
	REWRITE.c ROUND.c RSNG4.c SCLCK.c SEED.c SIN.c SQRT.c STLIM.c \
	SUBSC.c SUBSCZ.c SUBT.c SUCC.c TEOF.c TEOLN.c TIME.c TRUNC.c \
	UNIT.c UNPACK.c WRITEC.c WRITEF.c WRITES.c WRITLN.c

SYS =	ERROR.c GETNAME.c IOSYNC.c PCEXIT.c PCLOSE.c PCSTART.c \
	PFCLOSE.c PERROR.c PFLUSH.c PMFLUSH.c UNSYNC.c \
	blkclr.c blkcpy.c

EXTN =	APPEND.c SEEK.c TELL.c

HDRS =	whoami.h h00vars.h

OBJECTS =	ACTFILE.o ADDT.o ARGV.o ASRT.o ASRTS.o ATAN.o BUFF.o CARD.o \
	CASERNG.o CHR.o CLCK.o COS.o CTTOT.o DATE.o DEFNAME.o \
	DFDISPOSE.o DISPOSE.o EXCEPT.o EXP.o EXPO.o FCALL.o FLUSH.o \
	FNIL.o FRTN.o FSAV.o GET.o HALT.o IN.o INCT.o LINO.o \
	LLIMIT.o LN.o MAX.o MULT.o NAM.o NEW.o NIL.o PACK.o\
	PAGE.o PRED.o PUT.o RANDOM.o RANG4.o READ4.o READ8.o READC.o\
	READE.o READLN.o RELEQ.o RELNE.o RELSGE.o RELSGT.o RELSLE.o\
	RELSLT.o RELTGE.o RELTGT.o RELTLE.o RELTLT.o REMOVE.o RESET.o\
	REWRITE.o ROUND.o RSNG4.o SCLCK.o SEED.o SIN.o SQRT.o STLIM.o\
	SUBSC.o SUBSCZ.o SUBT.o SUCC.o TEOF.o TEOLN.o TIME.o TRUNC.o\
	UNIT.o UNPACK.o WRITEC.o WRITEF.o WRITES.o WRITLN.o

SYOBJ =	ERROR.o GETNAME.o IOSYNC.o PCEXIT.o PCLOSE.o PCSTART.o \
	PFCLOSE.o PERROR.o PFLUSH.o PMFLUSH.o UNSYNC.o \
	blkclr.o blkcpy.o

EXOBJ =	APPEND.o SEEK.o TELL.o

all: libpc libpc_p

libpc libpc_p: ${OBJECTS} ${SYOBJ} ${EXOBJ}
	cd profiled; ${AR} cru ../libpc_p ${OBJECTS} ${SYOBJ} ${EXOBJ}
	${RANLIB} libpc_p
	${AR} cru libpc ${OBJECTS} ${SYOBJ} ${EXOBJ}
	${RANLIB} libpc

tools2: libpc libpc_p
tools2 install:
	${INSTALL} -c libpc_p ${LIB}/libpc_p.a
	${RANLIB} ${LIB}/libpc_p.a
	${INSTALL} -c libpc ${LIB}/libpc.a
	${RANLIB} ${LIB}/libpc.a

pretools tools1:
	for i in ${OBJECTS} ${SYOBJ} ${EXOBJ}; do \
		echo $$i; \
		$(CC) -c ${CFLAGS} ../`basename $$i .o`.c; \
		mv $$i tmp.o; \
		$(LD) -x -r -o $$i tmp.o; \
	done
	${AR} cru libpc ${OBJECTS} ${SYOBJ} ${EXOBJ}
	rm -f ${OBJECTS} ${SYOBJ} ${EXOBJ} tmp.o
	${INSTALL} -c libpc ${LIB}/libpc.a
	${RANLIB} ${LIB}/libpc.a

clean$(MACHINE): cleanprofiled
cleanprofiled:
	-$(RM) -r profiled
include $(GMAKERULES)

.c.o:
	-@if [ ! -d profiled ] ; then \
		mkdir profiled; \
	fi
	$(CC) -c -p ${CFLAGS} ../$*.c
	mv $@ tmp.o
	-$(LD) -X -r -o profiled/$@ tmp.o
	$(CC) -c ${CFLAGS} ../$*.c
	mv $@ tmp.o
	-$(LD) -x -r -o $@ tmp.o

ACTFILE.o:	ACTFILE.c
ADDT.o:	ADDT.c
ARGV.o:	ARGV.c
ASRT.o:	ASRT.c
ASRTS.o:	ASRTS.c
ATAN.o:	ATAN.c
BUFF.o:	BUFF.c
CARD.o:	CARD.c
CASERNG.o:	CASERNG.c
CHR.o:	CHR.c
CLCK.o:	CLCK.c
COS.o:	COS.c
CTTOT.o:	CTTOT.c
DATE.o:	DATE.c
DEFNAME.o:	DEFNAME.c
DFDISPOSE.o:	DFDISPOSE.c
DISPOSE.o:	DISPOSE.c
EXCEPT.o:	EXCEPT.c
EXP.o:	EXP.c
EXPO.o:	EXPO.c
FCALL.o:	FCALL.c
FLUSH.o:	FLUSH.c
FNIL.o:	FNIL.c
FRTN.o:	FRTN.c
FSAV.o:	FSAV.c
GET.o:	GET.c
HALT.o:	HALT.c
IN.o:	IN.c
INCT.o:	INCT.c
LINO.o:	LINO.c
LLIMIT.o:	LLIMIT.c
LN.o:	LN.c
MAX.o:	MAX.c
MULT.o:	MULT.c
NAM.o:	NAM.c
NEW.o:	NEW.c
NIL.o:	NIL.c
PACK.o:	PACK.c
PAGE.o:	PAGE.c
PRED.o:	PRED.c
PUT.o:	PUT.c
RANDOM.o:	RANDOM.c
RANG4.o:	RANG4.c
READ4.o:	READ4.c
READ8.o:	READ8.c
READC.o:	READC.c
READE.o:	READE.c
READLN.o:	READLN.c
RELEQ.o:	RELEQ.c
RELNE.o:	RELNE.c
RELSGE.o:	RELSGE.c
RELSGT.o:	RELSGT.c
RELSLE.o:	RELSLE.c
RELSLT.o:	RELSLT.c
RELTGE.o:	RELTGE.c
RELTGT.o:	RELTGT.c
RELTLE.o:	RELTLE.c
RELTLT.o:	RELTLT.c
REMOVE.o:	REMOVE.c
RESET.o:	RESET.c
REWRITE.o:	REWRITE.c
ROUND.o:	ROUND.c
RSNG4.o:	RSNG4.c
SCLCK.o:	SCLCK.c
SEED.o:	SEED.c
SIN.o:	SIN.c
SQRT.o:	SQRT.c
STLIM.o:	STLIM.c
SUBSC.o:	SUBSC.c
SUBSCZ.o:	SUBSCZ.c
SUBT.o:	SUBT.c
SUCC.o:	SUCC.c
TEOF.o:	TEOF.c
TEOLN.o:	TEOLN.c
TIME.o:	TIME.c
TRUNC.o:	TRUNC.c
UNIT.o:	UNIT.c
UNPACK.o:	UNPACK.c
WRITEC.o:	WRITEC.c
WRITEF.o:	WRITEF.c
WRITES.o:	WRITES.c
WRITLN.o:	WRITLN.c
ERROR.o:	ERROR.c
GETNAME.o:	GETNAME.c
IOSYNC.o:	IOSYNC.c
PCEXIT.o:	PCEXIT.c
PCLOSE.o:	PCLOSE.c
PCSTART.o:	PCSTART.c
PFCLOSE.o:	PFCLOSE.c
PERROR.o:	PERROR.c
PFLUSH.o:	PFLUSH.c
PMFLUSH.o:	PMFLUSH.c
UNSYNC.o:	UNSYNC.c
blkclr.o:	blkclr.c
blkcpy.o:	blkcpy.c
APPEND.o:	APPEND.c
SEEK.o:	SEEK.c
TELL.o:	TELL.c

whoami.h:
#	whoami.h is in ..
#       To make this work with cp, we would need to
#   reassess the definition of PASDIR, which includes the use
#   of DESTROOT??!!, 
#	cp ${PASDIR}/whoami.h whoami.h

OLDprint:
	ls -l >lst
	${SRCDIR}/vprint lst Makefile *.h [A-Z][A-Z]*.[cs] [a-z][a-z]*.[cs]
	${RM} lst

OLDgrind: sources
	${SRCDIR}/ctags -v *.h *.c | sort -f >index
	${SRCDIR}/vgrind -t -x index >lpr
	${SRCDIR}/vgrind -t -n Makefile >>lpr
	${RM} index
	${SRCDIR}/vgrind -t *.h *.c >>lpr
	${SRCDIR}/vpr -t lpr

OLDdepend:	sources
	/bin/grep '^#[ 	]*include' ${CMDS} ${SYS} ${EXTN} | sed \
		-e '/<.*>/d' \
		-e 's/:[^"]*"\([^"]*\)".*/: \1/' \
		-e 's/\.c/.o/' >makedep
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

# DO NOT DELETE THIS LINE -- make depend uses it
# DEPENDENCIES MUST END AT END OF FILE
ACTFILE.o: h00vars.h
ARGV.o: h00vars.h
BUFF.o: h00vars.h
CTTOT.o: whoami.h
CTTOT.o: h00vars.h
DEFNAME.o: h00vars.h
DFDISPOSE.o: h00vars.h
DFDISPOSE.o: libpc.h
DISPOSE.o: h00vars.h
EXCEPT.o: whoami.h
FCALL.o: h00vars.h
FLUSH.o: h00vars.h
FNIL.o: h00vars.h
FRTN.o: h00vars.h
FSAV.o: h00vars.h
GET.o: h00vars.h
HALT.o: h00vars.h
IN.o: h00vars.h
INCT.o: h00vars.h
LINO.o: h00vars.h
LLIMIT.o: h00vars.h
MAX.o: h00vars.h
NAM.o: h00vars.h
NEW.o: h00vars.h
NIL.o: h00vars.h
PAGE.o: h00vars.h
PUT.o: h00vars.h
RANDOM.o: h00vars.h
READ4.o: h00vars.h
READ8.o: h00vars.h
READC.o: h00vars.h
READE.o: h00vars.h
READLN.o: h00vars.h
RELEQ.o: h00vars.h
RELNE.o: h00vars.h
RELSGE.o: h00vars.h
RELSGT.o: h00vars.h
RELSLE.o: h00vars.h
RELSLT.o: h00vars.h
RELTGE.o: h00vars.h
RELTGT.o: h00vars.h
RELTLE.o: h00vars.h
RELTLT.o: h00vars.h
REMOVE.o: h00vars.h
RESET.o: h00vars.h
REWRITE.o: h00vars.h
SEED.o: h00vars.h
STLIM.o: h00vars.h
TEOF.o: h00vars.h
TEOLN.o: h00vars.h
UNIT.o: h00vars.h
WRITEC.o: h00vars.h
WRITEF.o: h00vars.h
WRITES.o: h00vars.h
WRITLN.o: h00vars.h
GETNAME.o: h00vars.h
GETNAME.o: libpc.h
IOSYNC.o: h00vars.h
PCEXIT.o: h00vars.h
PCLOSE.o: h00vars.h
PCLOSE.o: libpc.h
PCSTART.o: h00vars.h
PCSTART.o: libpc.h
PFCLOSE.o: h00vars.h
PFCLOSE.o: libpc.h
PFLUSH.o: h00vars.h
PMFLUSH.o: h00vars.h
UNSYNC.o: h00vars.h
APPEND.o: h00vars.h
SEEK.o: h00vars.h
TELL.o: h00vars.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
