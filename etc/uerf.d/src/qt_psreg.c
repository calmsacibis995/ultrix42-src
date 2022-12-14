#ifndef lint
static char sccsid[]  =  "@(#)qt_psreg.c	4.1   (ULTRIX)   9/4/90";
#endif  lint
#include "qtrans.h"

struct qt_entry qt_psreg[] = {
	{ 1, 2, 0, 5, 4, 0, "OLD STATE - RIC51 - NO ERRORS"},
	{ 1, 2, 0, 5, 4, 1, "OLD STATE - RIC51 - RUNTIME ERROR"},
	{ 1, 2, 0, 5, 4, 2, "OLD STATE - RIC51 - XXNET ERRORS"},
	{ 1, 2, 0, 5, 4, 3, "OLD STATE - RIC51 - NOT RESPONDING"},
	{ 1, 2, 0, 3, 2, 0, "OLD STATE - RIC22 - NO ERRORS"},
	{ 1, 2, 0, 3, 2, 1, "OLD STATE - RIC22 - RUNTIME ERROR"},
	{ 1, 2, 0, 3, 2, 2, "OLD STATE - RIC22 - XXNET ERRORS"},
	{ 1, 2, 0, 3, 2, 3, "OLD STATE - RIC22 - NOT RESPONDING"},
	{ 1, 2, 0, 1, 0, 0, "OLD STATE - RIC12 - NO ERRORS"},
	{ 1, 2, 0, 1, 0, 1, "OLD STATE - RIC12 - RUNTIME ERROR"},
	{ 1, 2, 0, 1, 0, 2, "OLD STATE - RIC12 - XXNET ERRORS"},
	{ 1, 2, 0, 1, 0, 3, "OLD STATE - RIC12 - NOT RESPONDING"},
	{ 1, 3, 0, 7, 6, 0, "OLD STATE - RIC41 - NO ERRORS"},
	{ 1, 3, 0, 7, 6, 1, "OLD STATE - RIC41 - RUNTIME ERROR"},
	{ 1, 3, 0, 7, 6, 2, "OLD STATE - RIC41 - XXNET ERRORS"},
	{ 1, 3, 0, 7, 6, 3, "OLD STATE - RIC41 - NOT RESPONDING"},
	{ 1, 3, 0, 5, 4, 0, "OLD STATE - RIC31 - NO ERRORS"},
	{ 1, 3, 0, 5, 4, 1, "OLD STATE - RIC31 - RUNTIME ERROR"},
	{ 1, 3, 0, 5, 4, 2, "OLD STATE - RIC31 - XXNET ERRORS"},
	{ 1, 3, 0, 5, 4, 3, "OLD STATE - RIC31 - NOT RESPONDING"},
	{ 1, 3, 0, 3, 2, 0, "OLD STATE - RIC21 - NO ERRORS"},
	{ 1, 3, 0, 3, 2, 1, "OLD STATE - RIC21 - RUNTIME ERROR"},
	{ 1, 3, 0, 3, 2, 2, "OLD STATE - RIC21 - XXNET ERRORS"},
	{ 1, 3, 0, 3, 2, 3, "OLD STATE - RIC21 - NOT RESPONDING"},
	{ 1, 3, 0, 1, 0, 0, "OLD STATE - RIC11 - NO ERRORS"},
	{ 1, 3, 0, 1, 0, 1, "OLD STATE - RIC11 - RUNTIME ERROR"},
	{ 1, 3, 0, 1, 0, 2, "OLD STATE - RIC11 - XXNET ERRORS"},
	{ 1, 3, 0, 1, 0, 3, "OLD STATE - RIC11 - NOT RESPONDING"},
	{ 1, 4, 0, 5, 4, 0, "OLD STATE - RIC52 - NO ERRORS"},
	{ 1, 4, 0, 5, 4, 1, "OLD STATE - RIC52 - RUNTIME ERROR"},
	{ 1, 4, 0, 5, 4, 2, "OLD STATE - RIC52 - XXNET ERRORS"},
	{ 1, 4, 0, 5, 4, 3, "OLD STATE - RIC52 - NOT RESPONDING"},
	{ 1, 4, 0, 3, 2, 0, "OLD STATE - RIC23 - NO ERRORS"},
	{ 1, 4, 0, 3, 2, 1, "OLD STATE - RIC23 - RUNTIME ERROR"},
	{ 1, 4, 0, 3, 2, 2, "OLD STATE - RIC23 - XXNET ERRORS"},
	{ 1, 4, 0, 3, 2, 3, "OLD STATE - RIC23 - NOT RESPONDING"},
	{ 1, 4, 0, 1, 0, 0, "OLD STATE - RIC13 - NO ERRORS"},
	{ 1, 4, 0, 1, 0, 1, "OLD STATE - RIC13 - RUNTIME ERROR"},
	{ 1, 4, 0, 1, 0, 2, "OLD STATE - RIC13 - XXNET ERRORS"},
	{ 1, 4, 0, 1, 0, 3, "OLD STATE - RIC13 - NOT RESPONDING"},
	{ 1, 5, 0, 7, 6, 0, "OLD STATE - RIC42 - NO ERRORS"},
	{ 1, 5, 0, 7, 6, 1, "OLD STATE - RIC42 - RUNTIME ERROR"},
	{ 1, 5, 0, 7, 6, 2, "OLD STATE - RIC42 - XXNET ERRORS"},
	{ 1, 5, 0, 7, 6, 3, "OLD STATE - RIC42 - NOT RESPONDING"},
	{ 1, 5, 0, 5, 4, 0, "OLD STATE - RIC32 - NO ERRORS"},
	{ 1, 5, 0, 5, 4, 1, "OLD STATE - RIC32 - RUNTIME ERROR"},
	{ 1, 5, 0, 5, 4, 2, "OLD STATE - RIC32 - XXNET ERRORS"},
	{ 1, 5, 0, 5, 4, 3, "OLD STATE - RIC32 - NOT RESPONDING"},
	{ 1, 5, 0, 3, 2, 0, "OLD STATE - RIC24 - NO ERRORS"},
	{ 1, 5, 0, 3, 2, 1, "OLD STATE - RIC24 - RUNTIME ERROR"},
	{ 1, 5, 0, 3, 2, 2, "OLD STATE - RIC24 - XXNET ERRORS"},
	{ 1, 5, 0, 3, 2, 3, "OLD STATE - RIC24 - NOT RESPONDING"},
	{ 1, 5, 0, 1, 0, 0, "OLD STATE - RIC14 - NO ERRORS"},
	{ 1, 5, 0, 1, 0, 1, "OLD STATE - RIC14 - RUNTIME ERROR"},
	{ 1, 5, 0, 1, 0, 2, "OLD STATE - RIC14 - XXNET ERRORS"},
	{ 1, 5, 0, 1, 0, 3, "OLD STATE - RIC14 - NOT RESPONDING"},
	{ 1, 6, 0, 1, 0, 0, "OLD STATE - RIC53 - NO ERRORS"},
	{ 1, 6, 0, 1, 0, 1, "OLD STATE - RIC53 - RUNTIME ERROR"},
	{ 1, 6, 0, 1, 0, 2, "OLD STATE - RIC53 - XXNET ERRORS"},
	{ 1, 6, 0, 1, 0, 3, "OLD STATE - RIC53 - NOT RESPONDING"},
	{ 1, 7, 0, 5, 4, 0, "OLD STATE - RIC54 - NO ERRORS"},
	{ 1, 7, 0, 5, 4, 1, "OLD STATE - RIC54 - RUNTIME ERROR"},
	{ 1, 7, 0, 5, 4, 2, "OLD STATE - RIC54 - XXNET ERRORS"},
	{ 1, 7, 0, 5, 4, 3, "OLD STATE - RIC54 - NOT RESPONDING"},
	{ 1, 7, 0, 3, 2, 0, "OLD STATE - RIC25 - NO ERRORS"},
	{ 1, 7, 0, 3, 2, 1, "OLD STATE - RIC25 - RUNTIME ERROR"},
	{ 1, 7, 0, 3, 2, 2, "OLD STATE - RIC25 - XXNET ERRORS"},
	{ 1, 7, 0, 3, 2, 3, "OLD STATE - RIC25 - NOT RESPONDING"},
	{ 1, 7, 0, 1, 0, 0, "OLD STATE - RIC15 - NO ERRORS"},
	{ 1, 7, 0, 1, 0, 1, "OLD STATE - RIC15 - RUNTIME ERROR"},
	{ 1, 7, 0, 1, 0, 2, "OLD STATE - RIC15 - XXNET ERRORS"},
	{ 1, 7, 0, 1, 0, 3, "OLD STATE - RIC15 - NOT RESPONDING"},
	{ 1, 8, 0, 1, 1, 1, "OLD STATE - BBU #2 AVAILABLE"},
	{ 1, 8, 0, 1, 1, 0, "OLD STATE - BBU #2 NOT AVAILABLE"},
	{ 1, 8, 0, 0, 0, 1, "OLD STATE - BBU #1 AVAILABLE"},
	{ 1, 8, 0, 0, 0, 0, "OLD STATE - BBU #1 NOT AVAILABLE"},
	{ 1, 9, 0, 3, 2, 0, "OLD STATE - BBU #2 OFF"},
	{ 1, 9, 0, 3, 2, 1, "OLD STATE - BBU #2 CHARING"},
	{ 1, 9, 0, 3, 2, 2, "OLD STATE - BBU #2 FULLY CHARGED"},
	{ 1, 9, 0, 3, 2, 3, "OLD STATE - BBU #2 OPERATING"},
	{ 1, 9, 0, 1, 0, 0, "OLD STATE - BBU #1 OFF"},
	{ 1, 9, 0, 1, 0, 1, "OLD STATE - BBU #1 CHARING"},
	{ 1, 9, 0, 1, 0, 2, "OLD STATE - BBU #1 FULLY CHARGED"},
	{ 1, 9, 0, 1, 0, 3, "OLD STATE - BBU #1 OPERATING"},

	{ 1, 10, 0, 7, 7, 0, "OLD STATE - SW #2 - HALT"},
	{ 1, 10, 0, 6, 6, 0, "OLD STATE - SW #2 - RESTART/HALT"},
	{ 1, 10, 0, 5, 5, 0, "OLD STATE - SW #2 - RESTART/BOOT"},
	{ 1, 10, 0, 4, 4, 0, "OLD STATE - SW #2 - BOOT"},
	{ 1, 10, 0, 3, 3, 0, "OLD STATE - SW #1 - REMOTE"},
	{ 1, 10, 0, 2, 2, 0, "OLD STATE - SW #1 - REMOTE DISABLED"},
	{ 1, 10, 0, 1, 1, 0, "OLD STATE - SW #1 - LOCAL"},
	{ 1, 10, 0, 0, 0, 0, "OLD STATE - SW #1 - LOCAL DISABLED"},

	{ 1, 11, 12, 5, 2, 0xf, "OLD STATE - BIAS PSE MOD OK"},
	{ 1, 11, 13, 5, 2, 0xf, "OLD STATE - BIAS PSA MOD OK"},
	{ 1, 11, 12, 5, 2, -2, "OLD STATE - BIAS PSE NOT MOD OK"},
	{ 1, 11, 13, 5, 2, -2, "OLD STATE - BIAS PSA NOT MOD OK"},
	{ 1, 11, 0, 1, 1, 1, "OLD STATE - CONSOLE H7214 NOT OK"},
	{ 1, 11, 0, 1, 1, 0, "OLD STATE - CONSOLE H7214 OK"},
	{ 1, 11, 0, 0, 0, 1, "OLD STATE - CONSOLE H7215 NOT OK"},
	{ 1, 11, 0, 0, 0, 0, "OLD STATE - CONSOLE H7215 OK"},
	{ 1, 2, 0, 13, 12, 0, "NEW STATE - RIC51 - NO ERRORS"},
	{ 1, 2, 0, 13, 12, 1, "NEW STATE - RIC51 - RUNTIME ERROR"},
	{ 1, 2, 0, 13, 12, 2, "NEW STATE - RIC51 - XXNET ERRORS"},
	{ 1, 2, 0, 13, 12, 3, "NEW STATE - RIC51 - NOT RESPONDING"},
	{ 1, 2, 0, 11, 10, 0, "NEW STATE - RIC22 - NO ERRORS"},
	{ 1, 2, 0, 11, 10, 1, "NEW STATE - RIC22 - RUNTIME ERROR"},
	{ 1, 2, 0, 11, 10, 2, "NEW STATE - RIC22 - XXNET ERRORS"},
	{ 1, 2, 0, 11, 10, 3, "NEW STATE - RIC22 - NOT RESPONDING"},
	{ 1, 2, 0, 9, 8, 0, "NEW STATE - RIC12 - NO ERRORS"},
	{ 1, 2, 0, 9, 8, 1, "NEW STATE - RIC12 - RUNTIME ERROR"},
	{ 1, 2, 0, 9, 8, 2, "NEW STATE - RIC12 - XXNET ERRORS"},
	{ 1, 2, 0, 9, 8, 3, "NEW STATE - RIC12 - NOT RESPONDING"},
	{ 1, 3, 0, 15, 14, 0, "NEW STATE - RIC41 - NO ERRORS"},
	{ 1, 3, 0, 15, 14, 1, "NEW STATE - RIC41 - RUNTIME ERROR"},
	{ 1, 3, 0, 15, 14, 2, "NEW STATE - RIC41 - XXNET ERRORS"},
	{ 1, 3, 0, 15, 14, 3, "NEW STATE - RIC41 - NOT RESPONDING"},
	{ 1, 3, 0, 13, 12, 0, "NEW STATE - RIC31 - NO ERRORS"},
	{ 1, 3, 0, 13, 12, 1, "NEW STATE - RIC31 - RUNTIME ERROR"},
	{ 1, 3, 0, 13, 12, 2, "NEW STATE - RIC31 - XXNET ERRORS"},
	{ 1, 3, 0, 13, 12, 3, "NEW STATE - RIC31 - NOT RESPONDING"},
	{ 1, 3, 0, 11, 10, 0, "NEW STATE - RIC21 - NO ERRORS"},
	{ 1, 3, 0, 11, 10, 1, "NEW STATE - RIC21 - RUNTIME ERROR"},
	{ 1, 3, 0, 11, 10, 2, "NEW STATE - RIC21 - XXNET ERRORS"},
	{ 1, 3, 0, 11, 10, 3, "NEW STATE - RIC21 - NOT RESPONDING"},
	{ 1, 3, 0, 9, 8, 0, "NEW STATE - RIC11 - NO ERRORS"},
	{ 1, 3, 0, 9, 8, 1, "NEW STATE - RIC11 - RUNTIME ERROR"},
	{ 1, 3, 0, 9, 8, 2, "NEW STATE - RIC11 - XXNET ERRORS"},
	{ 1, 3, 0, 9, 8, 3, "NEW STATE - RIC11 - NOT RESPONDING"},
	{ 1, 4, 0, 13, 12, 0, "NEW STATE - RIC52 - NO ERRORS"},
	{ 1, 4, 0, 13, 12, 1, "OLD STATE - RIC52 - RUNTIME ERROR"},
	{ 1, 4, 0, 13, 12, 2, "OLD STATE - RIC52 - XXNET ERRORS"},
	{ 1, 4, 0, 13, 12, 3, "OLD STATE - RIC52 - NOT RESPONDING"},
	{ 1, 4, 0, 11, 10, 0, "NEW STATE - RIC23 - NO ERRORS"},
	{ 1, 4, 0, 11, 10, 1, "NEW STATE - RIC23 - RUNTIME ERROR"},
	{ 1, 4, 0, 11, 10, 2, "NEW STATE - RIC23 - XXNET ERRORS"},
	{ 1, 4, 0, 11, 10, 3, "NEW STATE - RIC23 - NOT RESPONDING"},
	{ 1, 4, 0, 9, 8, 0, "NEW STATE - RIC13 - NO ERRORS"},
	{ 1, 4, 0, 9, 8, 1, "NEW STATE - RIC13 - RUNTIME ERROR"},
	{ 1, 4, 0, 9, 8, 2, "NEW STATE - RIC13 - XXNET ERRORS"},
	{ 1, 4, 0, 9, 8, 3, "NEW STATE - RIC13 - NOT RESPONDING"},
	{ 1, 5, 0, 15, 14, 0, "NEW STATE - RIC42 - NO ERRORS"},
	{ 1, 5, 0, 15, 14, 1, "NEW STATE - RIC42 - RUNTIME ERROR"},
	{ 1, 5, 0, 15, 14, 2, "NEW STATE - RIC42 - XXNET ERRORS"},
	{ 1, 5, 0, 15, 14, 3, "NEW STATE - RIC42 - NOT RESPONDING"},
	{ 1, 5, 0, 13, 12, 0, "NEW STATE - RIC32 - NO ERRORS"},
	{ 1, 5, 0, 13, 12, 1, "NEW STATE - RIC32 - RUNTIME ERROR"},
	{ 1, 5, 0, 13, 12, 2, "NEW STATE - RIC32 - XXNET ERRORS"},
	{ 1, 5, 0, 13, 12, 3, "NEW STATE - RIC32 - NOT RESPONDING"},
	{ 1, 5, 0, 11, 10, 0, "NEW STATE - RIC24 - NO ERRORS"},
	{ 1, 5, 0, 11, 10, 1, "OLD STATE - RIC24 - RUNTIME ERROR"},
	{ 1, 5, 0, 11, 10, 2, "OLD STATE - RIC24 - XXNET ERRORS"},
	{ 1, 5, 0, 11, 10, 3, "OLD STATE - RIC24 - NOT RESPONDING"},
	{ 1, 5, 0, 9, 8, 0, "NEW STATE - RIC14 - NO ERRORS"},
	{ 1, 5, 0, 9, 8, 1, "NEW STATE - RIC14 - RUNTIME ERROR"},
	{ 1, 5, 0, 9, 8, 2, "NEW STATE - RIC14 - XXNET ERRORS"},
	{ 1, 5, 0, 9, 8, 3, "NEW STATE - RIC14 - NOT RESPONDING"},
	{ 1, 6, 0, 9, 8, 0, "NEW STATE - RIC53 - NO ERRORS"},
	{ 1, 6, 0, 9, 8, 1, "NEW STATE - RIC53 - RUNTIME ERROR"},
	{ 1, 6, 0, 9, 8, 2, "NEW STATE - RIC53 - XXNET ERRORS"},
	{ 1, 6, 0, 9, 8, 3, "NEW STATE - RIC53 - NOT RESPONDING"},
	{ 1, 7, 0, 13, 12, 0, "NEW STATE - RIC54 - NO ERRORS"},
	{ 1, 7, 0, 13, 12, 1, "NEW STATE - RIC54 - RUNTIME ERROR"},
	{ 1, 7, 0, 13, 12, 2, "NEW STATE - RIC54 - XXNET ERRORS"},
	{ 1, 7, 0, 13, 12, 3, "NEW STATE - RIC54 - NOT RESPONDING"},
	{ 1, 7, 0, 11, 10, 0, "NEW STATE - RIC25 - NO ERRORS"},
	{ 1, 7, 0, 11, 10, 1, "NEW STATE - RIC25 - RUNTIME ERROR"},
	{ 1, 7, 0, 11, 10, 2, "NEW STATE - RIC25 - XXNET ERRORS"},
	{ 1, 7, 0, 11, 10, 3, "NEW STATE - RIC25 - NOT RESPONDING"},
	{ 1, 7, 0, 9, 8, 0, "NEW STATE - RIC15 - NO ERRORS"},
	{ 1, 7, 0, 9, 8, 1, "OLD STATE - RIC15 - RUNTIME ERROR"},
	{ 1, 7, 0, 9, 8, 2, "OLD STATE - RIC15 - XXNET ERRORS"},
	{ 1, 7, 0, 9, 8, 3, "OLD STATE - RIC15 - NOT RESPONDING"},
	{ 1, 8, 0, 9, 9, 1, "NEW STATE - BBU #2 AVAILABLE"},
	{ 1, 8, 0, 9, 9, 0, "NEW STATE - BBU #2 NOT AVAILABLE"},
	{ 1, 8, 0, 8, 8, 1, "NEW STATE - BBU #1 AVAILABLE"},
	{ 1, 8, 0, 8, 8, 0, "NEW STATE - BBU #1 NOT AVAILABLE"},
	{ 1, 9, 0, 11, 10, 0, "NEW STATE - BBU #2 OFF"},
	{ 1, 9, 0, 11, 10, 1, "NEW STATE - BBU #2 CHARING"},
	{ 1, 9, 0, 11, 10, 2, "NEW STATE - BBU #2 FULLY CHARGED"},
	{ 1, 9, 0, 11, 10, 3, "NEW STATE - BBU #2 OPERATING"},
	{ 1, 9, 0, 9, 8, 0, "NEW STATE - BBU #1 OFF"},
	{ 1, 9, 0, 9, 8, 1, "NEW STATE - BBU #1 CHARING"},
	{ 1, 9, 0, 9, 8, 2, "NEW STATE - BBU #1 FULLY CHARGED"},
	{ 1, 9, 0, 9, 8, 3, "NEW STATE - BBU #1 OPERATING"},

	{ 1, 10, 0, 15, 15, 0, "NEW STATE - SW #2 - HALT"},
	{ 1, 10, 0, 14, 14, 0, "NEW STATE - SW #2 - RESTART/HALT"},
	{ 1, 10, 0, 13, 13, 0, "NEW STATE - SW #2 - RESTART/BOOT"},
	{ 1, 10, 0, 12, 12, 0, "NEW STATE - SW #2 - BOOT"},
	{ 1, 10, 0, 11, 11, 0, "NEW STATE - SW #1 - REMOTE"},
	{ 1, 10, 0, 10, 10, 0, "NEW STATE - SW #1 - REMOTE DISABLED"},
	{ 1, 10, 0, 9, 9, 0, "NEW STATE - SW #1 - LOCAL"},
	{ 1, 10, 0, 8, 8, 0, "NEW STATE - SW #1 - LOCAL DISABLED"},

	{ 1, 11, 12, 13, 10, 0xf, "NEW STATE - BIAS PSE MOD OK"},
	{ 1, 11, 13, 13, 10, 0xf, "NEW STATE - BIAS PSA MOD OK"},
	{ 1, 11, 12, 13, 10, -2, "NEW STATE - BIAS PSE NOT MOD OK"},
	{ 1, 11, 13, 13, 10, -2, "NEW STATE - BIAS PSA NOT MOD OK"},
	{ 1, 11, 12, 13, 10, 0, "NEW STATE - BIAS PSD NOT MOD OK"},
	{ 1, 11, 13, 13, 10, 0, "NEW STATE - BIAS PSC NOT MOD OK"},
	{ 1, 11, 0, 9, 9, 1, "NEW STATE - CONSOLE H7214 NOT OK"},
	{ 1, 11, 0, 9, 9, 0, "NEW STATE - CONSOLE H7214 OK"},
	{ 1, 11, 0, 8, 8, 1, "NEW STATE - CONSOLE H7215 NOT OK"},
	{ 1, 11, 0, 8, 8, 0, "NEW STATE - CONSOLE H7215 OK"},
	{ -1, 0, 0, 0, 0, 0, "" }
};
