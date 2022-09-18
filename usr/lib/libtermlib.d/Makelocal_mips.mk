#  @(#)Makelocal_mips.mk	4.1  ULTRIX  7/2/90
#
#mip-only rules
#
VGRIND=	csh /usr/ucb/vgrind
vgrind:
	cp /dev/null index
	${VGRIND} -h "Termcap library" termcap.c tputs.c tgoto.c
	${VGRIND} -h "Termcap library" -x index
