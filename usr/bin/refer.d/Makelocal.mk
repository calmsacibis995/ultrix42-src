# @(#)Makelocal.mk	4.1	ULTRIX	7/17/90
include $(GMAKEVARS)
#
#	@(#)Makefile	4.5	(Berkeley)	84/06/05

CFLAGS = -O
PUBLISTS = Rbstjissue Rv7man

all:	mkey inv hunt refer addbib lookbib sortbib runinv INDfiles

mkey: mkey1.o mkey2.o mkey3.o deliv2.o
	$(LDCMD) mkey?.o deliv2.o
inv: inv1.o inv2.o inv3.o inv5.o inv6.o deliv2.o
	$(LDCMD) inv?.o deliv2.o
hunt: hunt1.o hunt2.o hunt3.o hunt5.o hunt6.o hunt7.o glue5.o
hunt: refer3.o hunt9.o shell.o deliv2.o hunt8.o glue4.o tick.o
	$(LDCMD) hunt?.o refer3.o glue5.o glue4.o shell.o deliv2.o tick.o

runinv: ../runinv.sh
	cp ../runinv.sh runinv

# This rule makes Ind.i[abc]
INDfiles: ${PUBLISTS}
	-@$(RM) $(PUBLISTS)
	for i in ${PUBLISTS}; do \
	   cp ../$$i .; done
	sh runinv

glue3.o hunt2.o hunt3.o refer0.o refer1.o \
  refer2.o refer3.o refer4.o refer6.o refer5.o: ../refer..c
refer: glue1.o refer1.o refer2.o refer4.o refer5.o refer6.o mkey3.o
refer: refer7.o refer8.o hunt2.o hunt3.o deliv2.o hunt5.o hunt6.o hunt8.o
refer: glue3.o hunt7.o hunt9.o glue2.o glue4.o glue5.o refer0.o shell.o
	$(LDCMD) glue?.o refer[01245678].o hunt[2356789].o mkey3.o shell.o deliv2.o

addbib.o lookbib.o sortbib.o \
deliv1.o deliv2.o \
glue1.o glue2.o glue3.o glue4.o glue5.o \
hunt1.o hunt2.o hunt3.o hunt5.o hunt6.o hunt7.o hunt8.o hunt9.o \
inv1.o inv2.o inv3.o inv5.o inv6.o \
mkey1.o mkey2.o mkey3.o \
refer0.o refer1.o refer2.o refer3.o refer4.o \
refer5.o refer6.o refer7.o refer8.o \
shell.o tick.o \
what1.o what2.o what3.o what4.o:
	$(CCCMD) ../$(@:.o=.c)

addbib: addbib.o
	$(LDCMD) addbib.o
lookbib: lookbib.o
	$(LDCMD) lookbib.o
sortbib: sortbib.o
	$(LDCMD) sortbib.o

pretools tools1: mkey inv hunt ${DESTROOT}/usr/lib/refer
	install -c -s mkey $(DESTROOT)/usr/lib/refer
	install -c -s inv  $(DESTROOT)/usr/lib/refer
	install -c -s hunt $(DESTROOT)/usr/lib/refer

#	-if [ ! -d ${DESTROOT}/usr/lib/refer ]; then \
#		mkdir ${DESTROOT}/usr/lib/refer; \
#	else true; \
#	fi
${DESTROOT}/usr/lib/refer \
${DESTROOT}/usr/lib/tmac:
	-if [ ! -d $@ ]; then \
		mkdir $@; \
	else true; \
	fi

install: ${DESTROOT}/usr/lib/refer ${DESTROOT}/usr/lib/tmac
	install -c -s mkey $(DESTROOT)/usr/lib/refer/mkey
	install -c -s inv  $(DESTROOT)/usr/lib/refer/inv
	install -c -s hunt $(DESTROOT)/usr/lib/refer/hunt
	install -c -s refer $(DESTROOT)/usr/bin/refer
	install -s -c addbib $(DESTROOT)/usr/bin/addbib
	install -c -s lookbib $(DESTROOT)/usr/bin/lookbib
	install -c -s sortbib $(DESTROOT)/usr/bin/sortbib
	install -c ../roffbib.sh $(DESTROOT)/usr/bin/roffbib
	install -c ../indxbib.sh $(DESTROOT)/usr/bin/indxbib
	install -c ../tmac.bib $(DESTROOT)/usr/lib/tmac/tmac.bib

#
# Create the papers directory, if necessary.
# Build the index files for the publication lists
#
	-if [ ! -d ${DESTROOT}/usr/dict ]; \
	then \
		mkdir ${DESTROOT}/usr/dict; \
	else \
		true; \
	fi
	-if [ ! -d ${DESTROOT}/usr/dict/papers ]; \
	then \
		mkdir ${DESTROOT}/usr/dict/papers; \
	else \
		true; \
	fi
	for i in ${PUBLISTS} Ind.ia Ind.ib Ind.ic; do \
	    install -c $$i ${DESTROOT}/usr/dict/papers/$$i;\
	done
	install -c runinv ${DESTROOT}/usr/dict/papers


whatabout: what1.o what2.o what3.o what4.o shell.o mkey3.o
	$(LDCMD) what?.o shell.o mkey3.o
deliv: deliv1.o deliv2.o
	$(LDCMD) deliv?.o
refpart: refer0.o refer1.o refer2.o refer3.o refer4.o refer5.o
refpart: refer6.o refer7.o refer8.o deliv2.o glue4.o
	$(LDCMD) refer?.o deliv2.o glue4.o

addbib.o:	addbib.c
lookbib.o:	lookbib.c
sortbib.o:	sortbib.c
deliv1.o:	deliv1.c
deliv2.o:	deliv2.c
glue1.o:	glue1.c
glue2.o:	glue2.c
glue3.o:	glue3.c
glue4.o:	glue4.c
glue5.o:	glue5.c
hunt1.o:	hunt1.c
hunt2.o:	hunt2.c
hunt3.o:	hunt3.c
hunt5.o:	hunt5.c
hunt6.o:	hunt6.c
hunt7.o:	hunt7.c
hunt8.o:	hunt8.c
hunt9.o:	hunt9.c
inv1.o:	inv1.c
inv2.o:	inv2.c
inv3.o:	inv3.c
inv5.o:	inv5.c
inv6.o:	inv6.c
mkey1.o:	mkey1.c
mkey2.o:	mkey2.c
mkey3.o:	mkey3.c
refer0.o:	refer0.c
refer1.o:	refer1.c
refer2.o:	refer2.c
refer3.o:	refer3.c
refer4.o:	refer4.c
refer5.o:	refer5.c
refer6.o:	refer6.c
refer7.o:	refer7.c
refer8.o:	refer8.c
shell.o:	shell.c
tick.o:	tick.c
what1.o:	what1.c
what2.o:	what2.c
what3.o:	what3.c
what4.o:	what4.c

include $(GMAKERULES)
