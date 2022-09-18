#!/bin/sh
#  @(#)runinv.sh	4.1	ULTRIX	7/17/90
# Create index files for the publication lists
#
/usr/lib/refer/mkey R* | /usr/lib/refer/inv -v -h997 -n Xind
mv Xind.ia Ind.ia
mv Xind.ib Ind.ib
mv Xind.ic Ind.ic
