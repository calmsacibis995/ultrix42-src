/*
 * @(#)ISO646.inc	4.1
 * This is the definition of the code set as defined in ISO 646.
 *
 * As most extended ISO code sets contain ASCII in their lower half, this
 * is intended for inclusion in the definitions for the appropriate set.
 */

/*
 * control codes:
 */
nul = 0x00 : CTRL; 		soh = 0x01 : CTRL; 
stx = 0x02 : CTRL; 		etx = 0x03 : CTRL;
eot = 0x04 : CTRL; 		enq = 0x05 : CTRL; 
ack = 0x06 : CTRL; 		bel = 0x07 : CTRL;
bs  = 0x08 : CTRL; 		tab = 0x09 : SPACE, CTRL; 
lf  = 0x0a : SPACE, CTRL; 	vt  = 0x0b : SPACE, CTRL;
ff  = 0x0c : SPACE, CTRL; 	cr  = 0x0d : SPACE, CTRL; 
so  = 0x0e : CTRL; 		si  = 0x0f : CTRL;

dle = 0x10 : CTRL; 		dc1 = 0x11 : CTRL; 
dc2 = 0x12 : CTRL; 		dc3 = 0x13 : CTRL;
dc4 = 0x14 : CTRL; 		nak = 0x15 : CTRL; 
syn = 0x16 : CTRL; 		etb = 0x17 : CTRL;
can = 0x18 : CTRL; 		em  = 0x19 : CTRL; 
sub = 0x1a : CTRL; 		esc = 0x1b : CTRL;
fs  = 0x1c : CTRL; 		gs  = 0x1d : CTRL; 
rs  = 0x1e : CTRL; 		us  = 0x1f : CTRL;

/*
 * ASCII printable characters:
 */
space = ' '	: SPACE, BLANK;	
exclmark = '!'	: PUNCT;
quotes = '"'	: PUNCT;
/*
 * The codes 2/3 and 2/4 can be different in different languages, it
 * is therefore not defined in this general part.
 */
percent = '%'	: MISCEL, PUNCT;
at = '&'	: MISCEL, PUNCT;	
apostr = 2/7	: PUNCT;
leftpar = '('	: PUNCT;	
rightpar = ')'	: PUNCT;
asterisk = '*'	: MISCEL, PUNCT;	
plus = '+'	: ARITH, PUNCT;
comma = ','	: PUNCT;		
minus = '-'	: ARITH, PUNCT;
dot = '.'	: PUNCT;		
solidus = '/'	: PUNCT;

zero = '0'	: NUMERAL, HEX;	
one = '1'	: NUMERAL, HEX;
two = '2'	: NUMERAL, HEX;	
three = '3'	: NUMERAL, HEX;
four = '4'	: NUMERAL, HEX;	
five = '5'	: NUMERAL, HEX;
six = '6'	: NUMERAL, HEX;	
seven = '7'	: NUMERAL, HEX;
eight = '8'	: NUMERAL, HEX;	
nine = '9'	: NUMERAL, HEX;
colon = ':'  	: PUNCT;		
semi = ';'	: PUNCT;
less = '<'	: ARITH, PUNCT;		
equal = '='	: ARITH, PUNCT;
greater = '>'	: ARITH, PUNCT;		
questmark = '?'	: PUNCT;
/*
 * The code 4/0 can be different in different languages, it
 * is therefor not deffined in this general part.
 */
A = 'A'	: UPPER, HEX;
B = 'B'	: UPPER, HEX;		
C = 'C'	: UPPER, HEX;		
D = 'D'	: UPPER, HEX;		
E = 'E'	: UPPER, HEX;
F = 'F'	: UPPER, HEX;		
G = 'G'	: UPPER;		
H = 'H'	: UPPER;			
I = 'I'	: UPPER; 		
J = 'J'	: UPPER;			
K = 'K'	: UPPER;		
L = 'L'	: UPPER;			
M = 'M'	: UPPER; 		
N = 'N'	: UPPER;			
O = 'O'	: UPPER;

P = 'P'	: UPPER;		
Q = 'Q'	: UPPER;
R = 'R'	: UPPER;		
S = 'S'	: UPPER;
T = 'T'	: UPPER;		
U = 'U'	: UPPER;
V = 'V'	: UPPER;		
W = 'W'	: UPPER;
X = 'X'	: UPPER;		
Y = 'Y'	: UPPER;
Z = 'Z'	: UPPER;
/*
 * The codes 5/11, 5/12, 5/13 and 5/14 can be different in different languages, 
 * it is therefor not deffined in this general part.
 */
low_line = '_'	: PUNCT;

/*
 * The code 6/0 can be different in different languages, it
 * is therefor not deffined in this general part.
 */
a = 'a' : LOWER, HEX;
b = 'b'	: LOWER, HEX;		
c = 'c'	: LOWER, HEX;
d = 'd'	: LOWER, HEX;		
e = 'e'	: LOWER, HEX;
f = 'f'	: LOWER, HEX;		
g = 'g'	: LOWER;
h = 'h'	: LOWER;			
i = 'i'	: LOWER;
j = 'j'	: LOWER;			
k = 'k'	: LOWER;
l = 'l'	: LOWER;			
m = 'm'	: LOWER;
n = 'n'	: LOWER;			
o = 'o'	: LOWER;

p = 'p'	: LOWER;			
q = 'q'	: LOWER;
r = 'r'	: LOWER;			
s = 's'	: LOWER;
t = 't'	: LOWER;			
u = 'u'	: LOWER;
v = 'v'	: LOWER;			
w = 'w'	: LOWER;
x = 'x'	: LOWER;			
y = 'y'	: LOWER;
z = 'z'	: LOWER;
/*
 * The codes 7/11, 7/12, 7/13 and 7/14 can be different in different languages, 
 * it is therefor not deffined in this general part.
 */
del = 0x7f	: CTRL;
