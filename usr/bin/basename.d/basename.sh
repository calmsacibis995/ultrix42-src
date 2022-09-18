#! /bin/sh
# @(#)basename.sh	4.1	(ULTRIX)	7/17/90

AQQQ=${1-.}
AQQQ=`expr //$AQQQ : "\(.*\)\/$" \| $AQQQ`
BQQQ=`expr //$AQQQ : '.*/\(.*\)'`
expr $BQQQ : "\(.*\)$2$" \| $BQQQ
