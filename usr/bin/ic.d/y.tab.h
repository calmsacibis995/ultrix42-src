
typedef union  {
	sym		*y_sym;	/* Identifier */
	val		*y_val;	/* value: Constant or String or Identifier */
	i_char		 y_prp; /* property word */
} YYSTYPE;
extern YYSTYPE yylval;
# define CODESET 257
# define COLLATION 258
# define CONVERSION 259
# define DEFAULT 260
# define END 261
# define EXTENDED 262
# define STRINGTABLE 263
# define IS 264
# define PRIMARY 265
# define REST 266
# define SAME 267
# define EQUAL 268
# define VOID 269
# define CODE 270
# define STRING 271
# define PROPERTY 272
# define Identifier 273
# define Constant 274
# define String 275
# define Property 276
