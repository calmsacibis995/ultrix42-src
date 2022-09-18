#  @(#)Makelocal_vax.mk	4.1  ULTRIX  7/2/90

LINTDIR = ${DESTROOT}/usr/lib/lint

llib-lmalloc.ln: llib-lmall.c
	lint -Cmalloc -p ../llib-lmall.c
