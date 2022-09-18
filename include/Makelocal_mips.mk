#	@(#)Makelocal_mips.mk	4.1	(ULTRIX)	7/2/90
SYSDIRS=${NETDIRS} ${FSDIRS} ${TOPDIRS} sas
SUBDIRECTS=arpa pascal protocols cmplrs prom
MIPSLINKS=asm.h regdef.h
MACH-STD=alloca.h aouthdr.h disassembler.h exception.h filehdr.h fp_class.h \
	fpi.h ieeefp.h ldfcn.h linenum.h opnames.h reloc.h \
	scnhdr.h sex.h stamp.h storclass.h sym.h symconst.h syms.h sysmips.h
#
# Machine specific links
#
machlinks:
	-for i in ${MIPSLINKS}; do \
		rm -f ${DESTROOT}/usr/include/$$i; \
		ln -s machine/$$i ${DESTROOT}/usr/include/$$i; \
	done
