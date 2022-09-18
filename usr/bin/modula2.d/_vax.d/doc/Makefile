#@(#)Makefile	4.1	Ultrix	7/17/90
# $Header: Makefile,v 1.4 84/05/19 11:40:49 powell Exp $
# docs:	installation.f overview.f m+c+p.f mod.f p2m2.f
DESTROOT=
all:	installation.f overview.f m+c+p.f mod.f p2m2.f dbx.f
mod.f:
	nroff -man mod.l > mod.f
p2m2.f:
	nroff -man p2m2.l > p2m2.f
installation.f:
	nroff -ms installation > installation.f
overview.f:
	nroff -ms overview > overview.f
m+c+p.f:
	nroff -ms m+c+p > m+c+p.f
dbx.f:
	nroff -ms dbx > dbx.f
install:
	cp mod.l p2m2.l ${DESTROOT}/usr/man/manl
clean:
	-rm -f *.f
