TESTS = actst argtst chdtst ertst fktst gpctst idtst loctst seektst sigtst \
	statst timtst

all:	$(TESTS)

$(TESTS):
	f77 -O $*.f -o $*

clean:

clobber: clean


sccsinfo:
	sccs info

sccsget:
	sccs get SCCS
