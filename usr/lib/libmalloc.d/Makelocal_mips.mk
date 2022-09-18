#  @(#)Makelocal_mips.mk	4.2  ULTRIX  9/4/90

LINTDIR = ${DESTROOT}/usr/lib/lint

llib-lmalloc.ln: llib-lmall.c
	lint -c -p ../llib-lmall.c
	mv llib-lmall.ln llib-lmalloc.ln
