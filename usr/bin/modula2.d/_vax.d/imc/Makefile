#@(#)Makefile	4.1	Ultrix	7/17/90
# $Header: Makefile,v 1.4 84/05/19 11:33:12 powell Exp $
DESTROOT=
MODLIBDIR = ${DESTROOT}/usr/lib/mod
# TESTLIBDIR = /udir/powell/mod/lib
.SUFFIXES:
.SUFFIXES: .o .pcd .mod .def .c
CFLAGS = #-g
MFLAGS = -C #-g
OBJS = 
MOBJS =	imc.o execute.o list.o numtab.o porttab.o symtab.o stringtab.o

.def.mod:
	touch $*.mod
.mod.o:
	mod -c ${MFLAGS} $*.mod
imc:	$(MOBJS) libfile.o $(OBJS)
	mod -v -o imc $(CFLAGS) $(MOBJS) libfile.o $(OBJS)
test:	$(MOBJS) libfile.o $(OBJS)
	mod -v -M -c $(MOBJS) libfile.o $(OBJS)
testr:	$(MOBJS) libfile.o $(OBJS)
	mod -v -o imc -m"$(MFLAGS)" -c $(MOBJS) libfile.o $(OBJS)
iimc:	$(MOBJS) ilibfile.o $(OBJS)
	mod -v -o iimc $(CFLAGS) $(MOBJS) ilibfile.o $(OBJS)
ilibfile.mod:	Makefile
	echo "implementation module libfile;" >ilibfile.mod
	echo "begin" >>ilibfile.mod
	echo "    LIBFILENAME := '$(MODLIBDIR)/modlib';" >>ilibfile.mod
	echo "end libfile." >>ilibfile.mod
	
libfile.mod:	Makefile
	echo "implementation module libfile;" >libfile.mod
	echo "begin" >>libfile.mod
	echo "    LIBFILENAME := '$(TESTLIBDIR)/modlib';" >>libfile.mod
	echo "end libfile." >>libfile.mod
	
install: iimc
	cp iimc $(MODLIBDIR)/mod2.2
clean:
	-rm -f *.o
