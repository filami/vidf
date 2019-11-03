#ifndef YY_parse_h_included
#define YY_parse_h_included

#line 1 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.h"
/* before anything */
#ifdef c_plusplus
#ifndef __cplusplus
#define __cplusplus
#endif
#endif
#ifdef __cplusplus
#ifndef YY_USE_CLASS
#define YY_USE_CLASS
#endif
#else
#endif
#include <stdio.h>

/* #line 14 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.h" */
#line 21 "kcparser.h"
#line 1 "kcparser.y"

#include <malloc.h>
#include "kcscanner.h"
#include "kc.h"


#define YY_parse_CONSTRUCTOR_PARAM  FILE* in
#define YY_parse_CONSTRUCTOR_INIT  : Lexer(in), root(nullptr)
#define YY_parse_LEX_BODY  { return Lexer.yylex(); }
#define YY_parse_MEMBERS  lex Lexer; KCNode* root;

#line 29 "kcparser.y"
typedef union {
	KCNode* node;
} yy_parse_stype;
#define YY_parse_STYPE yy_parse_stype

#line 14 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.h"
 /* %{ and %header{ and %union, during decl */
#ifndef YY_parse_COMPATIBILITY
#ifndef YY_USE_CLASS
#define  YY_parse_COMPATIBILITY 1
#else
#define  YY_parse_COMPATIBILITY 0
#endif
#endif

#if YY_parse_COMPATIBILITY != 0
/* backward compatibility */
#ifdef YYLTYPE
#ifndef YY_parse_LTYPE
#define YY_parse_LTYPE YYLTYPE
/* WARNING obsolete !!! user defined YYLTYPE not reported into generated header */
/* use %define LTYPE */
#endif
#endif
#ifdef YYSTYPE
#ifndef YY_parse_STYPE 
#define YY_parse_STYPE YYSTYPE
/* WARNING obsolete !!! user defined YYSTYPE not reported into generated header */
/* use %define STYPE */
#endif
#endif
#ifdef YYDEBUG
#ifndef YY_parse_DEBUG
#define  YY_parse_DEBUG YYDEBUG
/* WARNING obsolete !!! user defined YYDEBUG not reported into generated header */
/* use %define DEBUG */
#endif
#endif
#ifdef YY_parse_STYPE
#ifndef yystype
#define yystype YY_parse_STYPE
#endif
#endif
/* use goto to be compatible */
#ifndef YY_parse_USE_GOTO
#define YY_parse_USE_GOTO 1
#endif
#endif

/* use no goto to be clean in C++ */
#ifndef YY_parse_USE_GOTO
#define YY_parse_USE_GOTO 0
#endif

#ifndef YY_parse_PURE

/* #line 63 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.h" */
#line 92 "kcparser.h"

#line 63 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.h"
/* YY_parse_PURE */
#endif

/* #line 65 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.h" */
#line 99 "kcparser.h"

#line 65 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.h"
/* prefix */
#ifndef YY_parse_DEBUG

/* #line 67 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.h" */
#line 106 "kcparser.h"

#line 67 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.h"
/* YY_parse_DEBUG */
#endif
#ifndef YY_parse_LSP_NEEDED

/* #line 70 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.h" */
#line 114 "kcparser.h"

#line 70 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.h"
 /* YY_parse_LSP_NEEDED*/
#endif
/* DEFAULT LTYPE*/
#ifdef YY_parse_LSP_NEEDED
#ifndef YY_parse_LTYPE
typedef
  struct yyltype
    {
      int timestamp;
      int first_line;
      int first_column;
      int last_line;
      int last_column;
      char *text;
   }
  yyltype;

#define YY_parse_LTYPE yyltype
#endif
#endif
/* DEFAULT STYPE*/
#ifndef YY_parse_STYPE
#define YY_parse_STYPE int
#endif
/* DEFAULT MISCELANEOUS */
#ifndef YY_parse_PARSE
#define YY_parse_PARSE yyparse
#endif
#ifndef YY_parse_LEX
#define YY_parse_LEX yylex
#endif
#ifndef YY_parse_LVAL
#define YY_parse_LVAL yylval
#endif
#ifndef YY_parse_LLOC
#define YY_parse_LLOC yylloc
#endif
#ifndef YY_parse_CHAR
#define YY_parse_CHAR yychar
#endif
#ifndef YY_parse_NERRS
#define YY_parse_NERRS yynerrs
#endif
#ifndef YY_parse_DEBUG_FLAG
#define YY_parse_DEBUG_FLAG yydebug
#endif
#ifndef YY_parse_ERROR
#define YY_parse_ERROR yyerror
#endif

#ifndef YY_parse_PARSE_PARAM
#ifndef __STDC__
#ifndef __cplusplus
#ifndef YY_USE_CLASS
#define YY_parse_PARSE_PARAM
#ifndef YY_parse_PARSE_PARAM_DEF
#define YY_parse_PARSE_PARAM_DEF
#endif
#endif
#endif
#endif
#ifndef YY_parse_PARSE_PARAM
#define YY_parse_PARSE_PARAM void
#endif
#endif

/* TOKEN C */
#ifndef YY_USE_CLASS

#ifndef YY_parse_PURE
extern YY_parse_STYPE YY_parse_LVAL;
#endif


/* #line 143 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.h" */
#line 192 "kcparser.h"
#define	IDENTIFIER	258
#define	CONSTANT	259
#define	STRING_LITERAL	260
#define	SIZEOF	261
#define	PTR_OP	262
#define	INC_OP	263
#define	DEC_OP	264
#define	LEFT_OP	265
#define	RIGHT_OP	266
#define	LE_OP	267
#define	GE_OP	268
#define	EQ_OP	269
#define	NE_OP	270
#define	AND_OP	271
#define	OR_OP	272
#define	MUL_ASSIGN	273
#define	DIV_ASSIGN	274
#define	MOD_ASSIGN	275
#define	ADD_ASSIGN	276
#define	SUB_ASSIGN	277
#define	LEFT_ASSIGN	278
#define	RIGHT_ASSIGN	279
#define	AND_ASSIGN	280
#define	XOR_ASSIGN	281
#define	OR_ASSIGN	282
#define	TYPE_NAME	283
#define	TYPEDEF	284
#define	EXTERN	285
#define	STATIC	286
#define	AUTO	287
#define	REGISTER	288
#define	CHAR	289
#define	SHORT	290
#define	INT	291
#define	LONG	292
#define	SIGNED	293
#define	UNSIGNED	294
#define	FLOAT	295
#define	DOUBLE	296
#define	CONST	297
#define	VOLATILE	298
#define	VOID	299
#define	STRUCT	300
#define	UNION	301
#define	ENUM	302
#define	ELLIPSIS	303
#define	CASE	304
#define	DEFAULT	305
#define	IF	306
#define	ELSE	307
#define	SWITCH	308
#define	WHILE	309
#define	DO	310
#define	FOR	311
#define	GOTO	312
#define	CONTINUE	313
#define	BREAK	314
#define	RETURN	315


#line 143 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.h"
 /* #defines token */
/* after #define tokens, before const tokens S5*/
#else
#ifndef YY_parse_CLASS
#define YY_parse_CLASS parse
#endif

#ifndef YY_parse_INHERIT
#define YY_parse_INHERIT
#endif
#ifndef YY_parse_MEMBERS
#define YY_parse_MEMBERS 
#endif
#ifndef YY_parse_LEX_BODY
#define YY_parse_LEX_BODY  
#endif
#ifndef YY_parse_ERROR_BODY
#define YY_parse_ERROR_BODY  
#endif
#ifndef YY_parse_CONSTRUCTOR_PARAM
#define YY_parse_CONSTRUCTOR_PARAM
#endif
/* choose between enum and const */
#ifndef YY_parse_USE_CONST_TOKEN
#define YY_parse_USE_CONST_TOKEN 0
/* yes enum is more compatible with flex,  */
/* so by default we use it */ 
#endif
#if YY_parse_USE_CONST_TOKEN != 0
#ifndef YY_parse_ENUM_TOKEN
#define YY_parse_ENUM_TOKEN yy_parse_enum_token
#endif
#endif

class YY_parse_CLASS YY_parse_INHERIT
{
public: 
#if YY_parse_USE_CONST_TOKEN != 0
/* static const int token ... */

/* #line 182 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.h" */
#line 295 "kcparser.h"
static const int IDENTIFIER;
static const int CONSTANT;
static const int STRING_LITERAL;
static const int SIZEOF;
static const int PTR_OP;
static const int INC_OP;
static const int DEC_OP;
static const int LEFT_OP;
static const int RIGHT_OP;
static const int LE_OP;
static const int GE_OP;
static const int EQ_OP;
static const int NE_OP;
static const int AND_OP;
static const int OR_OP;
static const int MUL_ASSIGN;
static const int DIV_ASSIGN;
static const int MOD_ASSIGN;
static const int ADD_ASSIGN;
static const int SUB_ASSIGN;
static const int LEFT_ASSIGN;
static const int RIGHT_ASSIGN;
static const int AND_ASSIGN;
static const int XOR_ASSIGN;
static const int OR_ASSIGN;
static const int TYPE_NAME;
static const int TYPEDEF;
static const int EXTERN;
static const int STATIC;
static const int AUTO;
static const int REGISTER;
static const int CHAR;
static const int SHORT;
static const int INT;
static const int LONG;
static const int SIGNED;
static const int UNSIGNED;
static const int FLOAT;
static const int DOUBLE;
static const int CONST;
static const int VOLATILE;
static const int VOID;
static const int STRUCT;
static const int UNION;
static const int ENUM;
static const int ELLIPSIS;
static const int CASE;
static const int DEFAULT;
static const int IF;
static const int ELSE;
static const int SWITCH;
static const int WHILE;
static const int DO;
static const int FOR;
static const int GOTO;
static const int CONTINUE;
static const int BREAK;
static const int RETURN;


#line 182 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.h"
 /* decl const */
#else
enum YY_parse_ENUM_TOKEN { YY_parse_NULL_TOKEN=0

/* #line 185 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.h" */
#line 362 "kcparser.h"
	,IDENTIFIER=258
	,CONSTANT=259
	,STRING_LITERAL=260
	,SIZEOF=261
	,PTR_OP=262
	,INC_OP=263
	,DEC_OP=264
	,LEFT_OP=265
	,RIGHT_OP=266
	,LE_OP=267
	,GE_OP=268
	,EQ_OP=269
	,NE_OP=270
	,AND_OP=271
	,OR_OP=272
	,MUL_ASSIGN=273
	,DIV_ASSIGN=274
	,MOD_ASSIGN=275
	,ADD_ASSIGN=276
	,SUB_ASSIGN=277
	,LEFT_ASSIGN=278
	,RIGHT_ASSIGN=279
	,AND_ASSIGN=280
	,XOR_ASSIGN=281
	,OR_ASSIGN=282
	,TYPE_NAME=283
	,TYPEDEF=284
	,EXTERN=285
	,STATIC=286
	,AUTO=287
	,REGISTER=288
	,CHAR=289
	,SHORT=290
	,INT=291
	,LONG=292
	,SIGNED=293
	,UNSIGNED=294
	,FLOAT=295
	,DOUBLE=296
	,CONST=297
	,VOLATILE=298
	,VOID=299
	,STRUCT=300
	,UNION=301
	,ENUM=302
	,ELLIPSIS=303
	,CASE=304
	,DEFAULT=305
	,IF=306
	,ELSE=307
	,SWITCH=308
	,WHILE=309
	,DO=310
	,FOR=311
	,GOTO=312
	,CONTINUE=313
	,BREAK=314
	,RETURN=315


#line 185 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.h"
 /* enum token */
     }; /* end of enum declaration */
#endif
public:
 int YY_parse_PARSE(YY_parse_PARSE_PARAM);
 virtual void YY_parse_ERROR(char *msg) YY_parse_ERROR_BODY;
#ifdef YY_parse_PURE
#ifdef YY_parse_LSP_NEEDED
 virtual int  YY_parse_LEX(YY_parse_STYPE *YY_parse_LVAL,YY_parse_LTYPE *YY_parse_LLOC) YY_parse_LEX_BODY;
#else
 virtual int  YY_parse_LEX(YY_parse_STYPE *YY_parse_LVAL) YY_parse_LEX_BODY;
#endif
#else
 virtual int YY_parse_LEX() YY_parse_LEX_BODY;
 YY_parse_STYPE YY_parse_LVAL;
#ifdef YY_parse_LSP_NEEDED
 YY_parse_LTYPE YY_parse_LLOC;
#endif
 int YY_parse_NERRS;
 int YY_parse_CHAR;
#endif
#if YY_parse_DEBUG != 0
public:
 int YY_parse_DEBUG_FLAG;	/*  nonzero means print parse trace	*/
#endif
public:
 YY_parse_CLASS(YY_parse_CONSTRUCTOR_PARAM);
public:
 YY_parse_MEMBERS 
};
/* other declare folow */
#endif


#if YY_parse_COMPATIBILITY != 0
/* backward compatibility */
#ifndef YYSTYPE
#define YYSTYPE YY_parse_STYPE
#endif

#ifndef YYLTYPE
#define YYLTYPE YY_parse_LTYPE
#endif
#ifndef YYDEBUG
#ifdef YY_parse_DEBUG 
#define YYDEBUG YY_parse_DEBUG
#endif
#endif

#endif
/* END */

/* #line 236 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.h" */
#line 477 "kcparser.h"
#endif