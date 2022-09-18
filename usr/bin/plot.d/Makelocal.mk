# @(#)Makelocal.mk	4.1	ULTRIX	7/17/90

include $(GMAKEVARS)

#	Makefile	4.3	83/07/04
# Modification History
#	April-10-1989, Pradeep Chetal
#	Added lots of new drivers from 4.3BSD-Tahoe release

LDFLAGS= -O

ALL=	tek t4013 t300 t300s t450 aedplot bgplot crtplot dumbplot gigiplot \
	hpplot hp7221plot implot atoplot plottoa grnplot vplot lvp16

all:	${ALL}

tek:	driver.o
	$(LDCMD)  driver.o -l4014 -lm

t4013:	driver.o
	$(LDCMD)  driver.o -l4013 -lm

t300:	driver.o 
	$(LDCMD) driver.o -l300 -lm

t300s:	driver.o 
	$(LDCMD) driver.o -l300s -lm

t450:	driver.o 
	$(LDCMD) driver.o -l450 -lm

aedplot:	driver.o
	$(LDCMD)  driver.o -lplotaed -lm

bgplot:	driver.o
	$(LDCMD)  driver.o -lplotbg -lm

crtplot:	crtdriver.o crtplot.o
	$(LDCMD)  crtdriver.o crtplot.o -lcurses -ltermcap -lm

dumbplot:	driver.o
	$(LDCMD)  driver.o -lplotdumb -ltermcap -lm

gigiplot:	driver.o
	$(LDCMD)  driver.o -lplotgigi -lm

hpplot:	driver.o
	$(LDCMD)  driver.o -lplot2648 -lm

hp7221plot:	driver.o
	$(LDCMD)  driver.o -lplot7221 -lm

implot:	driver.o
	$(LDCMD)  driver.o -lplotimagen -lm

atoplot:	atoplot.o
	$(LDCMD)  atoplot.o -lplot -lm

plottoa:	plottoa.o
	$(LDCMD)  plottoa.o

grnplot:	driver.o
	$(LDCMD)  driver.o -lplotgrn -lm

vplot:	vplot.o chrtab.o
	$(LDCMD) vplot.o chrtab.o

#
# lvp16 is compiled separately with -DLVP16 option
#
lvp16:	../driver.c
	$(LDCMD) -DLVP16 ../driver.c -llvp16 -lm

driver.o: ../driver.c
vplot.o:  ../vplot.c
chrtab.o: ../chrtab.c
atoplot.o: ../atoplot.c
plottoa.o: ../plottoa.c
crtdriver.o: ../crtdriver.c
crtplot.o: ../crtplot.c
driver.o vplot.o chrtab.o atoplot.o plottoa.o crtdriver.o crtplot.o:
	$(CCCMD) ../$(@:.o=.c)

install: all
	install -c -s tek ${DESTROOT}/usr/bin
	install -c -s t4013 ${DESTROOT}/usr/bin
	install -c -s t300 ${DESTROOT}/usr/bin
	install -c -s t300s ${DESTROOT}/usr/bin
	install -c -s t450 ${DESTROOT}/usr/bin
	install -c -s vplot ${DESTROOT}/usr/bin
	install -c -s aedplot ${DESTROOT}/usr/bin
	install -c -s bgplot ${DESTROOT}/usr/bin
	install -c -s crtplot ${DESTROOT}/usr/bin
	install -c -s dumbplot ${DESTROOT}/usr/bin
	install -c -s gigiplot ${DESTROOT}/usr/bin
	install -c -s hpplot ${DESTROOT}/usr/bin
	install -c -s hp7221plot ${DESTROOT}/usr/bin
	install -c -s implot ${DESTROOT}/usr/bin
	install -c -s atoplot ${DESTROOT}/usr/bin
	install -c -s plottoa ${DESTROOT}/usr/bin
	install -c -s grnplot ${DESTROOT}/usr/bin
	install -c -s lvp16 ${DESTROOT}/usr/bin
	install -c ../plot.sh ${DESTROOT}/usr/bin/plot

include $(GMAKERULES)
