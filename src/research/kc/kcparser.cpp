#define YY_parse_h_included

/*  A Bison++ parser, made from kcparser.y  */

 /* with Bison++ version bison++ Version 1.21-8, adapted from GNU bison by coetmeur@icdc.fr
  */


#line 1 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.cc"
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Bob Corbett and Richard Stallman

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 1, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* HEADER SECTION */
#if defined( _MSDOS ) || defined(MSDOS) || defined(__MSDOS__) 
#define __MSDOS_AND_ALIKE
#endif
#if defined(_WINDOWS) && defined(_MSC_VER)
#define __HAVE_NO_ALLOCA
#define __MSDOS_AND_ALIKE
#endif

#ifndef alloca
#if defined( __GNUC__)
#define alloca __builtin_alloca

#elif (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc)  || defined (__sgi)
#include <alloca.h>

#elif defined (__MSDOS_AND_ALIKE)
#include <malloc.h>
#ifndef __TURBOC__
/* MS C runtime lib */
#define alloca _alloca
#endif

#elif defined(_AIX)
#include <malloc.h>
#pragma alloca

#elif defined(__hpux)
#ifdef __cplusplus
extern "C" {
void *alloca (unsigned int);
};
#else /* not __cplusplus */
void *alloca ();
#endif /* not __cplusplus */

#endif /* not _AIX  not MSDOS, or __TURBOC__ or _AIX, not sparc.  */
#endif /* alloca not defined.  */
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
#ifndef __STDC__
#define const
#endif
#endif
#include <stdio.h>
#define YYBISON 1  

/* #line 73 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.cc" */
#line 85 "kcparser.cpp"
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

#line 73 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.cc"
/* %{ and %header{ and %union, during decl */
#define YY_parse_BISON 1
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
#endif
#endif
#ifdef YYSTYPE
#ifndef YY_parse_STYPE 
#define YY_parse_STYPE YYSTYPE
#endif
#endif
#ifdef YYDEBUG
#ifndef YY_parse_DEBUG
#define  YY_parse_DEBUG YYDEBUG
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

/* #line 117 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.cc" */
#line 151 "kcparser.cpp"

#line 117 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.cc"
/*  YY_parse_PURE */
#endif

/* section apres lecture def, avant lecture grammaire S2 */

/* #line 121 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.cc" */
#line 160 "kcparser.cpp"

#line 121 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.cc"
/* prefix */
#ifndef YY_parse_DEBUG

/* #line 123 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.cc" */
#line 167 "kcparser.cpp"

#line 123 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.cc"
/* YY_parse_DEBUG */
#endif


#ifndef YY_parse_LSP_NEEDED

/* #line 128 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.cc" */
#line 177 "kcparser.cpp"

#line 128 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.cc"
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
      /* We used to use `unsigned long' as YY_parse_STYPE on MSDOS,
	 but it seems better to be consistent.
	 Most programs should declare their own type anyway.  */

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
#if YY_parse_COMPATIBILITY != 0
/* backward compatibility */
#ifdef YY_parse_LTYPE
#ifndef YYLTYPE
#define YYLTYPE YY_parse_LTYPE
#else
/* WARNING obsolete !!! user defined YYLTYPE not reported into generated header */
#endif
#endif
#ifndef YYSTYPE
#define YYSTYPE YY_parse_STYPE
#else
/* WARNING obsolete !!! user defined YYSTYPE not reported into generated header */
#endif
#ifdef YY_parse_PURE
#ifndef YYPURE
#define YYPURE YY_parse_PURE
#endif
#endif
#ifdef YY_parse_DEBUG
#ifndef YYDEBUG
#define YYDEBUG YY_parse_DEBUG 
#endif
#endif
#ifndef YY_parse_ERROR_VERBOSE
#ifdef YYERROR_VERBOSE
#define YY_parse_ERROR_VERBOSE YYERROR_VERBOSE
#endif
#endif
#ifndef YY_parse_LSP_NEEDED
#ifdef YYLSP_NEEDED
#define YY_parse_LSP_NEEDED YYLSP_NEEDED
#endif
#endif
#endif
#ifndef YY_USE_CLASS
/* TOKEN C */

/* #line 236 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.cc" */
#line 290 "kcparser.cpp"
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


#line 236 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.cc"
 /* #defines tokens */
#else
/* CLASS */
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
#ifndef YY_parse_CONSTRUCTOR_CODE
#define YY_parse_CONSTRUCTOR_CODE
#endif
#ifndef YY_parse_CONSTRUCTOR_INIT
#define YY_parse_CONSTRUCTOR_INIT
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

/* #line 280 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.cc" */
#line 398 "kcparser.cpp"
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


#line 280 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.cc"
 /* decl const */
#else
enum YY_parse_ENUM_TOKEN { YY_parse_NULL_TOKEN=0

/* #line 283 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.cc" */
#line 465 "kcparser.cpp"
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


#line 283 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.cc"
 /* enum token */
     }; /* end of enum declaration */
#endif
public:
 int YY_parse_PARSE (YY_parse_PARSE_PARAM);
 virtual void YY_parse_ERROR(char *msg) YY_parse_ERROR_BODY;
#ifdef YY_parse_PURE
#ifdef YY_parse_LSP_NEEDED
 virtual int  YY_parse_LEX (YY_parse_STYPE *YY_parse_LVAL,YY_parse_LTYPE *YY_parse_LLOC) YY_parse_LEX_BODY;
#else
 virtual int  YY_parse_LEX (YY_parse_STYPE *YY_parse_LVAL) YY_parse_LEX_BODY;
#endif
#else
 virtual int YY_parse_LEX() YY_parse_LEX_BODY;
 YY_parse_STYPE YY_parse_LVAL;
#ifdef YY_parse_LSP_NEEDED
 YY_parse_LTYPE YY_parse_LLOC;
#endif
 int   YY_parse_NERRS;
 int    YY_parse_CHAR;
#endif
#if YY_parse_DEBUG != 0
 int YY_parse_DEBUG_FLAG;   /*  nonzero means print parse trace     */
#endif
public:
 YY_parse_CLASS(YY_parse_CONSTRUCTOR_PARAM);
public:
 YY_parse_MEMBERS 
};
/* other declare folow */
#if YY_parse_USE_CONST_TOKEN != 0

/* #line 314 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.cc" */
#line 560 "kcparser.cpp"
const int YY_parse_CLASS::IDENTIFIER=258;
const int YY_parse_CLASS::CONSTANT=259;
const int YY_parse_CLASS::STRING_LITERAL=260;
const int YY_parse_CLASS::SIZEOF=261;
const int YY_parse_CLASS::PTR_OP=262;
const int YY_parse_CLASS::INC_OP=263;
const int YY_parse_CLASS::DEC_OP=264;
const int YY_parse_CLASS::LEFT_OP=265;
const int YY_parse_CLASS::RIGHT_OP=266;
const int YY_parse_CLASS::LE_OP=267;
const int YY_parse_CLASS::GE_OP=268;
const int YY_parse_CLASS::EQ_OP=269;
const int YY_parse_CLASS::NE_OP=270;
const int YY_parse_CLASS::AND_OP=271;
const int YY_parse_CLASS::OR_OP=272;
const int YY_parse_CLASS::MUL_ASSIGN=273;
const int YY_parse_CLASS::DIV_ASSIGN=274;
const int YY_parse_CLASS::MOD_ASSIGN=275;
const int YY_parse_CLASS::ADD_ASSIGN=276;
const int YY_parse_CLASS::SUB_ASSIGN=277;
const int YY_parse_CLASS::LEFT_ASSIGN=278;
const int YY_parse_CLASS::RIGHT_ASSIGN=279;
const int YY_parse_CLASS::AND_ASSIGN=280;
const int YY_parse_CLASS::XOR_ASSIGN=281;
const int YY_parse_CLASS::OR_ASSIGN=282;
const int YY_parse_CLASS::TYPE_NAME=283;
const int YY_parse_CLASS::TYPEDEF=284;
const int YY_parse_CLASS::EXTERN=285;
const int YY_parse_CLASS::STATIC=286;
const int YY_parse_CLASS::AUTO=287;
const int YY_parse_CLASS::REGISTER=288;
const int YY_parse_CLASS::CHAR=289;
const int YY_parse_CLASS::SHORT=290;
const int YY_parse_CLASS::INT=291;
const int YY_parse_CLASS::LONG=292;
const int YY_parse_CLASS::SIGNED=293;
const int YY_parse_CLASS::UNSIGNED=294;
const int YY_parse_CLASS::FLOAT=295;
const int YY_parse_CLASS::DOUBLE=296;
const int YY_parse_CLASS::CONST=297;
const int YY_parse_CLASS::VOLATILE=298;
const int YY_parse_CLASS::VOID=299;
const int YY_parse_CLASS::STRUCT=300;
const int YY_parse_CLASS::UNION=301;
const int YY_parse_CLASS::ENUM=302;
const int YY_parse_CLASS::ELLIPSIS=303;
const int YY_parse_CLASS::CASE=304;
const int YY_parse_CLASS::DEFAULT=305;
const int YY_parse_CLASS::IF=306;
const int YY_parse_CLASS::ELSE=307;
const int YY_parse_CLASS::SWITCH=308;
const int YY_parse_CLASS::WHILE=309;
const int YY_parse_CLASS::DO=310;
const int YY_parse_CLASS::FOR=311;
const int YY_parse_CLASS::GOTO=312;
const int YY_parse_CLASS::CONTINUE=313;
const int YY_parse_CLASS::BREAK=314;
const int YY_parse_CLASS::RETURN=315;


#line 314 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.cc"
 /* const YY_parse_CLASS::token */
#endif
/*apres const  */
YY_parse_CLASS::YY_parse_CLASS(YY_parse_CONSTRUCTOR_PARAM) YY_parse_CONSTRUCTOR_INIT
{
#if YY_parse_DEBUG != 0
YY_parse_DEBUG_FLAG=0;
#endif
YY_parse_CONSTRUCTOR_CODE;
};
#endif

/* #line 325 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.cc" */
#line 635 "kcparser.cpp"


#define	YYFINAL		350
#define	YYFLAG		32768
#define	YYNTBASE	85

#define YYTRANSLATE(x) ((unsigned)(x) <= 315 ? yytranslate[x] : 148)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,    72,     2,     2,     2,    74,    67,     2,    61,
    62,    68,    69,    66,    70,    65,    73,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,    80,    82,    75,
    81,    76,    79,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
    63,     2,    64,    77,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,    83,    78,    84,    71,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     2,     3,     4,     5,
     6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
    16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
    26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
    36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
    46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
    56,    57,    58,    59,    60
};

#if YY_parse_DEBUG != 0
static const short yyprhs[] = {     0,
     0,     2,     4,     6,    10,    12,    17,    21,    26,    30,
    34,    37,    40,    42,    46,    48,    51,    54,    57,    60,
    65,    67,    69,    71,    73,    75,    77,    79,    84,    86,
    90,    94,    98,   100,   104,   108,   110,   114,   118,   120,
   124,   128,   132,   136,   138,   142,   146,   148,   152,   154,
   158,   160,   164,   166,   170,   172,   176,   178,   184,   186,
   190,   192,   194,   196,   198,   200,   202,   204,   206,   208,
   210,   212,   214,   218,   220,   223,   227,   229,   232,   234,
   237,   239,   242,   244,   248,   250,   254,   256,   258,   260,
   262,   264,   266,   268,   270,   272,   274,   276,   278,   280,
   282,   284,   286,   288,   294,   299,   302,   304,   306,   308,
   311,   315,   318,   320,   323,   325,   327,   331,   333,   336,
   340,   345,   351,   354,   356,   360,   362,   366,   368,   370,
   373,   375,   377,   381,   386,   390,   395,   400,   404,   406,
   409,   412,   416,   418,   421,   423,   427,   429,   433,   436,
   439,   441,   443,   447,   449,   452,   454,   456,   459,   463,
   466,   470,   474,   479,   482,   486,   490,   495,   497,   501,
   506,   508,   512,   514,   516,   518,   520,   522,   524,   528,
   533,   537,   540,   544,   548,   553,   555,   558,   560,   563,
   565,   568,   574,   582,   588,   594,   602,   609,   617,   621,
   624,   627,   630,   634,   636,   639,   641,   643,   648,   652,
   656
};

static const short yyrhs[] = {     3,
     0,     4,     0,     5,     0,    61,   104,    62,     0,    85,
     0,    86,    63,   104,    64,     0,    86,    61,    62,     0,
    86,    61,    87,    62,     0,    86,    65,     3,     0,    86,
     7,     3,     0,    86,     8,     0,    86,     9,     0,   102,
     0,    87,    66,   102,     0,    86,     0,     8,    88,     0,
     9,    88,     0,    89,    90,     0,     6,    88,     0,     6,
    61,   131,    62,     0,    67,     0,    68,     0,    69,     0,
    70,     0,    71,     0,    72,     0,    88,     0,    61,   131,
    62,    90,     0,    90,     0,    91,    68,    90,     0,    91,
    73,    90,     0,    91,    74,    90,     0,    91,     0,    92,
    69,    91,     0,    92,    70,    91,     0,    92,     0,    93,
    10,    92,     0,    93,    11,    92,     0,    93,     0,    94,
    75,    93,     0,    94,    76,    93,     0,    94,    12,    93,
     0,    94,    13,    93,     0,    94,     0,    95,    14,    94,
     0,    95,    15,    94,     0,    95,     0,    96,    67,    95,
     0,    96,     0,    97,    77,    96,     0,    97,     0,    98,
    78,    97,     0,    98,     0,    99,    16,    98,     0,    99,
     0,   100,    17,    99,     0,   100,     0,   100,    79,   104,
    80,   101,     0,   101,     0,    88,   103,   102,     0,    81,
     0,    18,     0,    19,     0,    20,     0,    21,     0,    22,
     0,    23,     0,    24,     0,    25,     0,    26,     0,    27,
     0,   102,     0,   104,    66,   102,     0,   101,     0,   107,
    82,     0,   107,   108,    82,     0,   110,     0,   110,   107,
     0,   111,     0,   111,   107,     0,   122,     0,   122,   107,
     0,   109,     0,   108,    66,   109,     0,   123,     0,   123,
    81,   134,     0,    29,     0,    30,     0,    31,     0,    32,
     0,    33,     0,    44,     0,    34,     0,    35,     0,    36,
     0,    37,     0,    40,     0,    41,     0,    38,     0,    39,
     0,   112,     0,   119,     0,    28,     0,   113,     3,    83,
   114,    84,     0,   113,    83,   114,    84,     0,   113,     3,
     0,    45,     0,    46,     0,   115,     0,   114,   115,     0,
   116,   117,    82,     0,   111,   116,     0,   111,     0,   122,
   116,     0,   122,     0,   118,     0,   117,    66,   118,     0,
   123,     0,    80,   105,     0,   123,    80,   105,     0,    47,
    83,   120,    84,     0,    47,     3,    83,   120,    84,     0,
    47,     3,     0,   121,     0,   120,    66,   121,     0,     3,
     0,     3,    81,   105,     0,    42,     0,    43,     0,   125,
   124,     0,   124,     0,     3,     0,    61,   123,    62,     0,
   124,    63,   105,    64,     0,   124,    63,    64,     0,   124,
    61,   127,    62,     0,   124,    61,   130,    62,     0,   124,
    61,    62,     0,    68,     0,    68,   126,     0,    68,   125,
     0,    68,   126,   125,     0,   122,     0,   126,   122,     0,
   128,     0,   128,    66,    48,     0,   129,     0,   128,    66,
   129,     0,   107,   123,     0,   107,   132,     0,   107,     0,
     3,     0,   130,    66,     3,     0,   116,     0,   116,   132,
     0,   125,     0,   133,     0,   125,   133,     0,    61,   132,
    62,     0,    63,    64,     0,    63,   105,    64,     0,   133,
    63,    64,     0,   133,    63,   105,    64,     0,    61,    62,
     0,    61,   127,    62,     0,   133,    61,    62,     0,   133,
    61,   127,    62,     0,   102,     0,    83,   135,    84,     0,
    83,   135,    66,    84,     0,   134,     0,   135,    66,   134,
     0,   137,     0,   138,     0,   141,     0,   142,     0,   143,
     0,   144,     0,     3,    80,   136,     0,    49,   105,    80,
   136,     0,    50,    80,   136,     0,    83,    84,     0,    83,
   140,    84,     0,    83,   139,    84,     0,    83,   139,   140,
    84,     0,   106,     0,   139,   106,     0,   136,     0,   140,
   136,     0,    82,     0,   104,    82,     0,    51,    61,   104,
    62,   136,     0,    51,    61,   104,    62,   136,    52,   136,
     0,    53,    61,   104,    62,   136,     0,    54,    61,   104,
    62,   136,     0,    55,   136,    54,    61,   104,    62,    82,
     0,    56,    61,   141,   141,    62,   136,     0,    56,    61,
   141,   141,   104,    62,   136,     0,    57,     3,    82,     0,
    58,    82,     0,    59,    82,     0,    60,    82,     0,    60,
   104,    82,     0,   146,     0,   145,   146,     0,   147,     0,
   106,     0,   107,   123,   139,   138,     0,   107,   123,   138,
     0,   123,   139,   138,     0,   123,   138,     0
};

#endif

#if YY_parse_DEBUG != 0
static const short yyrline[] = { 0,
    38,    39,    40,    41,    45,    46,    47,    48,    49,    50,
    51,    52,    56,    57,    61,    62,    63,    64,    65,    66,
    70,    71,    72,    73,    74,    75,    79,    80,    84,    85,
    86,    87,    91,    92,    93,    97,    98,    99,   103,   104,
   105,   106,   107,   111,   112,   113,   117,   118,   122,   123,
   127,   128,   132,   133,   137,   138,   142,   143,   147,   148,
   152,   153,   154,   155,   156,   157,   158,   159,   160,   161,
   162,   166,   167,   171,   175,   176,   180,   181,   182,   183,
   184,   185,   189,   190,   194,   195,   199,   200,   201,   202,
   203,   207,   208,   209,   210,   211,   212,   213,   214,   215,
   216,   217,   218,   222,   223,   224,   228,   229,   233,   234,
   238,   242,   243,   244,   245,   249,   250,   254,   255,   256,
   260,   261,   262,   266,   267,   271,   272,   276,   277,   281,
   282,   286,   287,   288,   289,   290,   291,   292,   296,   297,
   298,   299,   303,   304,   309,   310,   314,   315,   319,   320,
   321,   325,   326,   330,   331,   335,   336,   337,   341,   342,
   343,   344,   345,   346,   347,   348,   349,   353,   354,   355,
   359,   360,   364,   365,   366,   367,   368,   369,   373,   374,
   375,   379,   380,   381,   382,   386,   387,   391,   392,   396,
   397,   401,   402,   403,   407,   408,   409,   410,   414,   415,
   416,   417,   418,   422,   423,   427,   428,   432,   433,   434,
   435
};

static const char * const yytname[] = {   "$","error","$illegal.","IDENTIFIER",
"CONSTANT","STRING_LITERAL","SIZEOF","PTR_OP","INC_OP","DEC_OP","LEFT_OP","RIGHT_OP",
"LE_OP","GE_OP","EQ_OP","NE_OP","AND_OP","OR_OP","MUL_ASSIGN","DIV_ASSIGN","MOD_ASSIGN",
"ADD_ASSIGN","SUB_ASSIGN","LEFT_ASSIGN","RIGHT_ASSIGN","AND_ASSIGN","XOR_ASSIGN",
"OR_ASSIGN","TYPE_NAME","TYPEDEF","EXTERN","STATIC","AUTO","REGISTER","CHAR",
"SHORT","INT","LONG","SIGNED","UNSIGNED","FLOAT","DOUBLE","CONST","VOLATILE",
"VOID","STRUCT","UNION","ENUM","ELLIPSIS","CASE","DEFAULT","IF","ELSE","SWITCH",
"WHILE","DO","FOR","GOTO","CONTINUE","BREAK","RETURN","'('","')'","'['","']'",
"'.'","','","'&'","'*'","'+'","'-'","'~'","'!'","'/'","'%'","'<'","'>'","'^'",
"'|'","'?'","':'","'='","';'","'{'","'}'","primary_expression","postfix_expression",
"argument_expression_list","unary_expression","unary_operator","cast_expression",
"multiplicative_expression","additive_expression","shift_expression","relational_expression",
"equality_expression","and_expression","exclusive_or_expression","inclusive_or_expression",
"logical_and_expression","logical_or_expression","conditional_expression","assignment_expression",
"assignment_operator","expression","constant_expression","declaration","declaration_specifiers",
"init_declarator_list","init_declarator","storage_class_specifier","type_specifier",
"struct_or_union_specifier","struct_or_union","struct_declaration_list","struct_declaration",
"specifier_qualifier_list","struct_declarator_list","struct_declarator","enum_specifier",
"enumerator_list","enumerator","type_qualifier","declarator","direct_declarator",
"pointer","type_qualifier_list","parameter_type_list","parameter_list","parameter_declaration",
"identifier_list","type_name","abstract_declarator","direct_abstract_declarator",
"initializer","initializer_list","statement","labeled_statement","compound_statement",
"declaration_list","statement_list","expression_statement","selection_statement",
"iteration_statement","jump_statement","translation_unit","external_declaration",
"function_definition",""
};
#endif

static const short yyr1[] = {     0,
    85,    85,    85,    85,    86,    86,    86,    86,    86,    86,
    86,    86,    87,    87,    88,    88,    88,    88,    88,    88,
    89,    89,    89,    89,    89,    89,    90,    90,    91,    91,
    91,    91,    92,    92,    92,    93,    93,    93,    94,    94,
    94,    94,    94,    95,    95,    95,    96,    96,    97,    97,
    98,    98,    99,    99,   100,   100,   101,   101,   102,   102,
   103,   103,   103,   103,   103,   103,   103,   103,   103,   103,
   103,   104,   104,   105,   106,   106,   107,   107,   107,   107,
   107,   107,   108,   108,   109,   109,   110,   110,   110,   110,
   110,   111,   111,   111,   111,   111,   111,   111,   111,   111,
   111,   111,   111,   112,   112,   112,   113,   113,   114,   114,
   115,   116,   116,   116,   116,   117,   117,   118,   118,   118,
   119,   119,   119,   120,   120,   121,   121,   122,   122,   123,
   123,   124,   124,   124,   124,   124,   124,   124,   125,   125,
   125,   125,   126,   126,   127,   127,   128,   128,   129,   129,
   129,   130,   130,   131,   131,   132,   132,   132,   133,   133,
   133,   133,   133,   133,   133,   133,   133,   134,   134,   134,
   135,   135,   136,   136,   136,   136,   136,   136,   137,   137,
   137,   138,   138,   138,   138,   139,   139,   140,   140,   141,
   141,   142,   142,   142,   143,   143,   143,   143,   144,   144,
   144,   144,   144,   145,   145,   146,   146,   147,   147,   147,
   147
};

static const short yyr2[] = {     0,
     1,     1,     1,     3,     1,     4,     3,     4,     3,     3,
     2,     2,     1,     3,     1,     2,     2,     2,     2,     4,
     1,     1,     1,     1,     1,     1,     1,     4,     1,     3,
     3,     3,     1,     3,     3,     1,     3,     3,     1,     3,
     3,     3,     3,     1,     3,     3,     1,     3,     1,     3,
     1,     3,     1,     3,     1,     3,     1,     5,     1,     3,
     1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
     1,     1,     3,     1,     2,     3,     1,     2,     1,     2,
     1,     2,     1,     3,     1,     3,     1,     1,     1,     1,
     1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
     1,     1,     1,     5,     4,     2,     1,     1,     1,     2,
     3,     2,     1,     2,     1,     1,     3,     1,     2,     3,
     4,     5,     2,     1,     3,     1,     3,     1,     1,     2,
     1,     1,     3,     4,     3,     4,     4,     3,     1,     2,
     2,     3,     1,     2,     1,     3,     1,     3,     2,     2,
     1,     1,     3,     1,     2,     1,     1,     2,     3,     2,
     3,     3,     4,     2,     3,     3,     4,     1,     3,     4,
     1,     3,     1,     1,     1,     1,     1,     1,     3,     4,
     3,     2,     3,     3,     4,     1,     2,     1,     2,     1,
     2,     5,     7,     5,     5,     7,     6,     7,     3,     2,
     2,     2,     3,     1,     2,     1,     1,     4,     3,     3,
     2
};

static const short yydefact[] = {     0,
   132,   103,    87,    88,    89,    90,    91,    93,    94,    95,
    96,    99,   100,    97,    98,   128,   129,    92,   107,   108,
     0,     0,   139,   207,     0,    77,    79,   101,     0,   102,
    81,     0,   131,     0,     0,   204,   206,   123,     0,     0,
   143,   141,   140,    75,     0,    83,    85,    78,    80,   106,
     0,    82,     0,   186,     0,   211,     0,     0,     0,   130,
   205,     0,   126,     0,   124,   133,   144,   142,     0,    76,
     0,   209,     0,     0,   113,     0,   109,     0,   115,     1,
     2,     3,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,    21,    22,    23,
    24,    25,    26,   190,   182,     5,    15,    27,     0,    29,
    33,    36,    39,    44,    47,    49,    51,    53,    55,    57,
    59,    72,     0,   188,   173,   174,     0,     0,   175,   176,
   177,   178,    85,   187,   210,   152,   138,   151,     0,   145,
   147,     0,     1,   135,    27,    74,     0,     0,     0,     0,
   121,    84,     0,   168,    86,   208,     0,   112,   105,   110,
     0,     0,   116,   118,   114,     0,     0,    19,     0,    16,
    17,     0,     0,     0,     0,     0,     0,     0,     0,   200,
   201,   202,     0,     0,   154,     0,     0,    11,    12,     0,
     0,     0,    62,    63,    64,    65,    66,    67,    68,    69,
    70,    71,    61,     0,    18,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,   191,   184,     0,   183,   189,
     0,     0,   149,   156,   150,   157,   136,     0,   137,     0,
   134,   122,   127,   125,   171,     0,   104,   119,     0,   111,
     0,   179,     0,     0,   181,     0,     0,     0,     0,     0,
   199,   203,     4,     0,   156,   155,     0,    10,     7,     0,
    13,     0,     9,    60,    30,    31,    32,    34,    35,    37,
    38,    42,    43,    40,    41,    45,    46,    48,    50,    52,
    54,    56,     0,    73,   185,   164,     0,     0,   160,     0,
   158,     0,     0,   146,   148,   153,     0,   169,   117,   120,
    20,   180,     0,     0,     0,     0,     0,    28,     8,     0,
     6,     0,   165,   159,   161,   166,     0,   162,     0,   170,
   172,   192,   194,   195,     0,     0,     0,    14,    58,   167,
   163,     0,     0,   197,     0,   193,   196,   198,     0,     0
};

static const short yydefgoto[] = {   106,
   107,   270,   108,   109,   110,   111,   112,   113,   114,   115,
   116,   117,   118,   119,   120,   121,   122,   204,   123,   147,
    54,    55,    45,    46,    26,    27,    28,    29,    76,    77,
    78,   162,   163,    30,    64,    65,    31,    32,    33,    34,
    43,   297,   140,   141,   142,   186,   298,   236,   155,   246,
   124,   125,   126,    57,   128,   129,   130,   131,   132,    35,
    36,    37
};

static const short yypact[] = {   937,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
     5,    14,   -14,-32768,    12,  1261,  1261,-32768,     8,-32768,
  1261,  1108,   169,    48,   847,-32768,-32768,   -61,    49,     3,
-32768,-32768,   -14,-32768,   -10,-32768,  1088,-32768,-32768,   -13,
  1062,-32768,   288,-32768,    12,-32768,  1108,   982,   604,   169,
-32768,    49,    -4,    35,-32768,-32768,-32768,-32768,    14,-32768,
   534,-32768,  1108,  1062,  1062,  1011,-32768,    31,  1062,    32,
-32768,-32768,   773,   785,   785,   798,    70,    23,    66,   132,
   500,   135,   112,   119,   133,   570,   658,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,    60,   426,   798,-32768,
    78,    28,   237,    11,   243,   176,   159,   149,   235,     4,
-32768,-32768,    38,-32768,-32768,-32768,   358,   428,-32768,-32768,
-32768,-32768,   174,-32768,-32768,-32768,-32768,    13,   191,   201,
-32768,    87,-32768,-32768,-32768,-32768,   209,    40,   798,    49,
-32768,-32768,   534,-32768,-32768,-32768,  1031,-32768,-32768,-32768,
   798,    47,-32768,   194,-32768,   500,   658,-32768,   798,-32768,
-32768,   196,   500,   798,   798,   798,   223,   588,   206,-32768,
-32768,-32768,    74,   126,   123,   216,   286,-32768,-32768,   674,
   798,   292,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,   798,-32768,   798,   798,   798,   798,   798,
   798,   798,   798,   798,   798,   798,   798,   798,   798,   798,
   798,   798,   798,   798,   798,-32768,-32768,   464,-32768,-32768,
   892,   703,-32768,    42,-32768,   172,-32768,  1240,-32768,   295,
-32768,-32768,-32768,-32768,-32768,    44,-32768,-32768,    31,-32768,
   798,-32768,   238,   500,-32768,   128,   137,   150,   240,   588,
-32768,-32768,-32768,  1164,   183,-32768,   798,-32768,-32768,   151,
-32768,   160,-32768,-32768,-32768,-32768,-32768,    78,    78,    28,
    28,   237,   237,   237,   237,    11,    11,   243,   176,   159,
   149,   235,   -36,-32768,-32768,-32768,   241,   242,-32768,   245,
   172,  1205,   744,-32768,-32768,-32768,   139,-32768,-32768,-32768,
-32768,-32768,   500,   500,   500,   798,   757,-32768,-32768,   798,
-32768,   798,-32768,-32768,-32768,-32768,   244,-32768,   246,-32768,
-32768,   247,-32768,-32768,   152,   500,   163,-32768,-32768,-32768,
-32768,   500,   225,-32768,   500,-32768,-32768,-32768,   305,-32768
};

static const short yypgoto[] = {-32768,
-32768,-32768,   -47,-32768,   -90,    50,    54,    68,    53,    92,
    93,    91,   114,   117,-32768,   -54,   -68,-32768,   -37,   -53,
     6,     0,-32768,   281,-32768,   -26,-32768,-32768,   277,   -67,
   -33,-32768,    65,-32768,   253,   202,   211,   -12,   -32,    -3,
-32768,   -57,-32768,   115,-32768,   187,  -124,  -187,  -149,-32768,
   -73,-32768,   148,   136,   248,  -171,-32768,-32768,-32768,-32768,
   330,-32768
};


#define	YYLAST		1308


static const short yytable[] = {    25,
   139,    60,   154,   245,   146,    24,   260,    38,   160,    40,
    50,   145,    47,   235,     1,     1,     1,   177,   205,    42,
   223,    62,   213,   214,    75,    48,    49,    16,    17,   225,
    52,   146,   172,     1,    25,   168,   170,   171,   145,    68,
    24,   158,   133,   322,     1,   165,   301,    75,    75,    75,
     1,    63,    75,    23,   230,    69,   133,   138,   183,   184,
   266,   145,   134,   185,    66,   164,   187,   188,   189,    74,
    75,    70,    22,   231,    22,   232,   149,   301,   134,    23,
    23,    23,   224,   174,   154,   215,   216,    39,   317,   160,
    51,    22,   252,    44,   146,   243,   209,   210,    23,   255,
   150,   145,   231,   225,   232,   150,   146,   248,    22,   307,
   161,   166,   249,   145,   179,   275,   276,   277,   151,   226,
   190,   271,   191,   242,   192,   233,   175,   308,   250,   184,
    75,   184,   134,   185,   234,   274,   256,   257,   258,   225,
    75,   143,    81,    82,    83,   206,    84,    85,   239,   173,
   207,   208,   240,   272,   230,   262,   294,   331,   145,   145,
   145,   145,   145,   145,   145,   145,   145,   145,   145,   145,
   145,   145,   145,   145,   145,   145,   318,   146,   300,    56,
   312,   265,    73,   264,   145,   232,   293,   263,   127,   313,
    23,   225,   176,   225,    72,   178,   146,   310,   314,    97,
   180,    60,   225,   145,   135,    98,    99,   100,   101,   102,
   103,   315,   319,   343,   181,   225,   320,   225,    40,   145,
   156,   153,   330,   321,   345,   225,   221,   234,   225,    58,
   138,    59,   302,    41,   303,   220,   164,   138,   154,   332,
   333,   334,   219,   264,   327,   232,   211,   212,   146,   329,
   222,   338,   237,    67,    71,   145,   217,   218,   278,   279,
   265,    79,   344,   138,   280,   281,   238,   339,   346,   286,
   287,   348,   241,   251,   145,   254,   259,   267,   335,   337,
   282,   283,   284,   285,    79,    79,    79,   261,   268,    79,
    80,    81,    82,    83,   273,    84,    85,   306,   342,   311,
   316,   138,   323,   324,   350,   340,   347,    79,   325,   341,
   288,   290,   289,   309,   148,     2,     3,     4,     5,     6,
     7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
    17,    18,    19,    20,    21,   291,    86,    87,    88,   292,
    89,    90,    91,    92,    93,    94,    95,    96,    97,   152,
   157,   244,   305,   253,    98,    99,   100,   101,   102,   103,
    80,    81,    82,    83,    61,    84,    85,    79,     0,   104,
    53,   105,     0,     0,   228,     0,     0,    79,     0,     0,
     0,     0,     0,     0,     0,     2,     3,     4,     5,     6,
     7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
    17,    18,    19,    20,    21,     0,    86,    87,    88,     0,
    89,    90,    91,    92,    93,    94,    95,    96,    97,     0,
     0,     0,     0,     0,    98,    99,   100,   101,   102,   103,
    80,    81,    82,    83,     0,    84,    85,     0,     0,   104,
    53,   227,     0,   193,   194,   195,   196,   197,   198,   199,
   200,   201,   202,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,    80,    81,    82,    83,
     0,    84,    85,     0,     0,     0,    86,    87,    88,     0,
    89,    90,    91,    92,    93,    94,    95,    96,    97,     0,
     0,     0,     0,     0,    98,    99,   100,   101,   102,   103,
     0,     0,    80,    81,    82,    83,   203,    84,    85,   104,
    53,   229,    86,    87,    88,     0,    89,    90,    91,    92,
    93,    94,    95,    96,    97,     0,     0,     0,     0,     0,
    98,    99,   100,   101,   102,   103,   143,    81,    82,    83,
     0,    84,    85,     0,     0,   104,    53,   295,    86,    87,
    88,     0,    89,    90,    91,    92,    93,    94,    95,    96,
    97,     0,     0,     0,     0,     0,    98,    99,   100,   101,
   102,   103,   143,    81,    82,    83,     0,    84,    85,     0,
     0,   104,    53,     0,     0,     0,     0,     0,     0,     0,
   143,    81,    82,    83,    97,    84,    85,     0,     0,     0,
    98,    99,   100,   101,   102,   103,   143,    81,    82,    83,
     0,    84,    85,     0,     0,     0,   153,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    97,     0,     0,     0,     0,     0,    98,    99,   100,   101,
   102,   103,     0,     0,     0,     0,     0,     0,    97,     0,
     0,   182,     0,     0,    98,    99,   100,   101,   102,   103,
   143,    81,    82,    83,    97,    84,    85,   144,     0,   104,
    98,    99,   100,   101,   102,   103,   143,    81,    82,    83,
     0,    84,    85,     0,     0,     2,     0,     0,     0,     0,
     0,     8,     9,    10,    11,    12,    13,    14,    15,    16,
    17,    18,    19,    20,    21,   143,    81,    82,    83,     0,
    84,    85,     0,     0,     0,     0,     0,     0,    97,     0,
     0,     0,     0,     0,    98,    99,   100,   101,   102,   103,
     0,     0,     0,     0,    97,   269,     0,     0,     0,     0,
    98,    99,   100,   101,   102,   103,   143,    81,    82,    83,
     0,    84,    85,     0,     0,     0,     0,     0,     0,   143,
    81,    82,    83,    97,    84,    85,   299,     0,     0,    98,
    99,   100,   101,   102,   103,   143,    81,    82,    83,     0,
    84,    85,     0,     0,     0,     0,     0,   143,    81,    82,
    83,     0,    84,    85,     0,     0,     0,     0,     0,     0,
   143,    81,    82,    83,    97,    84,    85,   328,     0,     0,
    98,    99,   100,   101,   102,   103,     0,    97,   336,     0,
     0,     0,     0,    98,    99,   100,   101,   102,   103,     0,
     0,     0,     0,   167,     0,     0,     0,     0,     0,    98,
    99,   100,   101,   102,   103,   169,   349,     0,     0,     1,
     0,    98,    99,   100,   101,   102,   103,     0,    97,     0,
     0,     0,     0,     0,    98,    99,   100,   101,   102,   103,
     0,     0,     0,     0,     2,     3,     4,     5,     6,     7,
     8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
    18,    19,    20,    21,     1,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,    22,     0,     0,
     0,     0,     0,     0,    23,     0,     0,     0,     0,     2,
     3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
    13,    14,    15,    16,    17,    18,    19,    20,    21,     1,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,   231,   296,   232,     0,     0,     0,     0,    23,
     0,     0,     0,     0,     2,     3,     4,     5,     6,     7,
     8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
    18,    19,    20,    21,   136,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,    22,     0,     0,
     0,     0,     0,     0,    23,     0,     0,     0,     0,     2,
     3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
    13,    14,    15,    16,    17,    18,    19,    20,    21,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     2,     0,
     0,     0,     0,   137,     8,     9,    10,    11,    12,    13,
    14,    15,    16,    17,    18,    19,    20,    21,     2,     0,
     0,     0,     0,     0,     8,     9,    10,    11,    12,    13,
    14,    15,    16,    17,    18,    19,    20,    21,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     2,
     0,     0,     0,     0,   159,     8,     9,    10,    11,    12,
    13,    14,    15,    16,    17,    18,    19,    20,    21,     0,
     0,     0,     0,     0,   247,     2,     3,     4,     5,     6,
     7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
    17,    18,    19,    20,    21,     2,     3,     4,     5,     6,
     7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
    17,    18,    19,    20,    21,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,    71,     0,
    53,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    53,     2,     3,     4,     5,     6,     7,     8,     9,    10,
    11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
    21,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,   264,   296,   232,     0,     0,     0,
     0,    23,     2,     3,     4,     5,     6,     7,     8,     9,
    10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
    20,    21,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,   326,     2,     3,     4,
     5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
    15,    16,    17,    18,    19,    20,    21,   304,     2,     3,
     4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
    14,    15,    16,    17,    18,    19,    20,    21
};

static const short yycheck[] = {     0,
    58,    34,    71,   153,    59,     0,   178,     3,    76,    22,
     3,    59,    25,   138,     3,     3,     3,    91,   109,    23,
    17,    83,    12,    13,    51,    26,    27,    42,    43,    66,
    31,    86,    86,     3,    35,    83,    84,    85,    86,    43,
    35,    75,    55,    80,     3,    79,   234,    74,    75,    76,
     3,     3,    79,    68,   128,    66,    69,    58,    96,    97,
   185,   109,    57,    97,    62,    78,     7,     8,     9,    83,
    97,    82,    61,    61,    61,    63,    81,   265,    73,    68,
    68,    68,    79,    61,   153,    75,    76,    83,   260,   157,
    83,    61,   166,    82,   149,   149,    69,    70,    68,   173,
    66,   149,    61,    66,    63,    66,   161,   161,    61,    66,
    80,    80,    66,   161,     3,   206,   207,   208,    84,    82,
    61,   190,    63,    84,    65,   138,    61,    84,    82,   167,
   157,   169,   127,   167,   138,   204,   174,   175,   176,    66,
   167,     3,     4,     5,     6,    68,     8,     9,    62,    80,
    73,    74,    66,   191,   228,    82,   225,   307,   206,   207,
   208,   209,   210,   211,   212,   213,   214,   215,   216,   217,
   218,   219,   220,   221,   222,   223,   267,   232,   232,    32,
   254,   185,    47,    61,   232,    63,   224,    62,    53,    62,
    68,    66,    61,    66,    47,    61,   251,   251,    62,    61,
    82,   234,    66,   251,    57,    67,    68,    69,    70,    71,
    72,    62,    62,    62,    82,    66,    66,    66,   231,   267,
    73,    83,    84,    64,    62,    66,    78,   231,    66,    61,
   231,    63,    61,    23,    63,    77,   249,   238,   307,   313,
   314,   315,    67,    61,   302,    63,    10,    11,   303,   303,
    16,   320,    62,    43,    81,   303,    14,    15,   209,   210,
   264,    51,   336,   264,   211,   212,    66,   322,   342,   217,
   218,   345,    64,    80,   322,    80,    54,    62,   316,   317,
   213,   214,   215,   216,    74,    75,    76,    82,     3,    79,
     3,     4,     5,     6,     3,     8,     9,     3,    52,    62,
    61,   302,    62,    62,     0,    62,    82,    97,    64,    64,
   219,   221,   220,   249,    62,    28,    29,    30,    31,    32,
    33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
    43,    44,    45,    46,    47,   222,    49,    50,    51,   223,
    53,    54,    55,    56,    57,    58,    59,    60,    61,    69,
    74,   150,   238,   167,    67,    68,    69,    70,    71,    72,
     3,     4,     5,     6,    35,     8,     9,   157,    -1,    82,
    83,    84,    -1,    -1,   127,    -1,    -1,   167,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    28,    29,    30,    31,    32,
    33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
    43,    44,    45,    46,    47,    -1,    49,    50,    51,    -1,
    53,    54,    55,    56,    57,    58,    59,    60,    61,    -1,
    -1,    -1,    -1,    -1,    67,    68,    69,    70,    71,    72,
     3,     4,     5,     6,    -1,     8,     9,    -1,    -1,    82,
    83,    84,    -1,    18,    19,    20,    21,    22,    23,    24,
    25,    26,    27,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,     3,     4,     5,     6,
    -1,     8,     9,    -1,    -1,    -1,    49,    50,    51,    -1,
    53,    54,    55,    56,    57,    58,    59,    60,    61,    -1,
    -1,    -1,    -1,    -1,    67,    68,    69,    70,    71,    72,
    -1,    -1,     3,     4,     5,     6,    81,     8,     9,    82,
    83,    84,    49,    50,    51,    -1,    53,    54,    55,    56,
    57,    58,    59,    60,    61,    -1,    -1,    -1,    -1,    -1,
    67,    68,    69,    70,    71,    72,     3,     4,     5,     6,
    -1,     8,     9,    -1,    -1,    82,    83,    84,    49,    50,
    51,    -1,    53,    54,    55,    56,    57,    58,    59,    60,
    61,    -1,    -1,    -1,    -1,    -1,    67,    68,    69,    70,
    71,    72,     3,     4,     5,     6,    -1,     8,     9,    -1,
    -1,    82,    83,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     3,     4,     5,     6,    61,     8,     9,    -1,    -1,    -1,
    67,    68,    69,    70,    71,    72,     3,     4,     5,     6,
    -1,     8,     9,    -1,    -1,    -1,    83,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    61,    -1,    -1,    -1,    -1,    -1,    67,    68,    69,    70,
    71,    72,    -1,    -1,    -1,    -1,    -1,    -1,    61,    -1,
    -1,    82,    -1,    -1,    67,    68,    69,    70,    71,    72,
     3,     4,     5,     6,    61,     8,     9,    64,    -1,    82,
    67,    68,    69,    70,    71,    72,     3,     4,     5,     6,
    -1,     8,     9,    -1,    -1,    28,    -1,    -1,    -1,    -1,
    -1,    34,    35,    36,    37,    38,    39,    40,    41,    42,
    43,    44,    45,    46,    47,     3,     4,     5,     6,    -1,
     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,    61,    -1,
    -1,    -1,    -1,    -1,    67,    68,    69,    70,    71,    72,
    -1,    -1,    -1,    -1,    61,    62,    -1,    -1,    -1,    -1,
    67,    68,    69,    70,    71,    72,     3,     4,     5,     6,
    -1,     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,     3,
     4,     5,     6,    61,     8,     9,    64,    -1,    -1,    67,
    68,    69,    70,    71,    72,     3,     4,     5,     6,    -1,
     8,     9,    -1,    -1,    -1,    -1,    -1,     3,     4,     5,
     6,    -1,     8,     9,    -1,    -1,    -1,    -1,    -1,    -1,
     3,     4,     5,     6,    61,     8,     9,    64,    -1,    -1,
    67,    68,    69,    70,    71,    72,    -1,    61,    62,    -1,
    -1,    -1,    -1,    67,    68,    69,    70,    71,    72,    -1,
    -1,    -1,    -1,    61,    -1,    -1,    -1,    -1,    -1,    67,
    68,    69,    70,    71,    72,    61,     0,    -1,    -1,     3,
    -1,    67,    68,    69,    70,    71,    72,    -1,    61,    -1,
    -1,    -1,    -1,    -1,    67,    68,    69,    70,    71,    72,
    -1,    -1,    -1,    -1,    28,    29,    30,    31,    32,    33,
    34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
    44,    45,    46,    47,     3,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    61,    -1,    -1,
    -1,    -1,    -1,    -1,    68,    -1,    -1,    -1,    -1,    28,
    29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
    39,    40,    41,    42,    43,    44,    45,    46,    47,     3,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    61,    62,    63,    -1,    -1,    -1,    -1,    68,
    -1,    -1,    -1,    -1,    28,    29,    30,    31,    32,    33,
    34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
    44,    45,    46,    47,     3,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    61,    -1,    -1,
    -1,    -1,    -1,    -1,    68,    -1,    -1,    -1,    -1,    28,
    29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
    39,    40,    41,    42,    43,    44,    45,    46,    47,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    28,    -1,
    -1,    -1,    -1,    62,    34,    35,    36,    37,    38,    39,
    40,    41,    42,    43,    44,    45,    46,    47,    28,    -1,
    -1,    -1,    -1,    -1,    34,    35,    36,    37,    38,    39,
    40,    41,    42,    43,    44,    45,    46,    47,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    28,
    -1,    -1,    -1,    -1,    84,    34,    35,    36,    37,    38,
    39,    40,    41,    42,    43,    44,    45,    46,    47,    -1,
    -1,    -1,    -1,    -1,    84,    28,    29,    30,    31,    32,
    33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
    43,    44,    45,    46,    47,    28,    29,    30,    31,    32,
    33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
    43,    44,    45,    46,    47,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    81,    -1,
    83,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    83,    28,    29,    30,    31,    32,    33,    34,    35,    36,
    37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
    47,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    61,    62,    63,    -1,    -1,    -1,
    -1,    68,    28,    29,    30,    31,    32,    33,    34,    35,
    36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
    46,    47,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    62,    28,    29,    30,
    31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
    41,    42,    43,    44,    45,    46,    47,    48,    28,    29,
    30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
    40,    41,    42,    43,    44,    45,    46,    47
};

#line 325 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.cc"
 /* fattrs + tables */

/* parser code folow  */


/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

/* Note: dollar marks section change
   the next  is replaced by the list of actions, each action
   as one case of the switch.  */ 

#if YY_parse_USE_GOTO != 0
/* 
 SUPRESSION OF GOTO : on some C++ compiler (sun c++)
  the goto is strictly forbidden if any constructor/destructor
  is used in the whole function (very stupid isn't it ?)
 so goto are to be replaced with a 'while/switch/case construct'
 here are the macro to keep some apparent compatibility
*/
#define YYGOTO(lb) {yy_gotostate=lb;continue;}
#define YYBEGINGOTO  enum yy_labels yy_gotostate=yygotostart; \
                     for(;;) switch(yy_gotostate) { case yygotostart: {
#define YYLABEL(lb) } case lb: {
#define YYENDGOTO } } 
#define YYBEGINDECLARELABEL enum yy_labels {yygotostart
#define YYDECLARELABEL(lb) ,lb
#define YYENDDECLARELABEL  };
#else
/* macro to keep goto */
#define YYGOTO(lb) goto lb
#define YYBEGINGOTO 
#define YYLABEL(lb) lb:
#define YYENDGOTO
#define YYBEGINDECLARELABEL 
#define YYDECLARELABEL(lb)
#define YYENDDECLARELABEL 
#endif
/* LABEL DECLARATION */
YYBEGINDECLARELABEL
  YYDECLARELABEL(yynewstate)
  YYDECLARELABEL(yybackup)
/* YYDECLARELABEL(yyresume) */
  YYDECLARELABEL(yydefault)
  YYDECLARELABEL(yyreduce)
  YYDECLARELABEL(yyerrlab)   /* here on detecting error */
  YYDECLARELABEL(yyerrlab1)   /* here on error raised explicitly by an action */
  YYDECLARELABEL(yyerrdefault)  /* current state does not do anything special for the error token. */
  YYDECLARELABEL(yyerrpop)   /* pop the current state because it cannot handle the error token */
  YYDECLARELABEL(yyerrhandle)  
YYENDDECLARELABEL
/* ALLOCA SIMULATION */
/* __HAVE_NO_ALLOCA */
#ifdef __HAVE_NO_ALLOCA
int __alloca_free_ptr(char *ptr,char *ref)
{if(ptr!=ref) free(ptr);
 return 0;}

#define __ALLOCA_alloca(size) malloc(size)
#define __ALLOCA_free(ptr,ref) __alloca_free_ptr((char *)ptr,(char *)ref)

#ifdef YY_parse_LSP_NEEDED
#define __ALLOCA_return(num) \
            return( __ALLOCA_free(yyss,yyssa)+\
		    __ALLOCA_free(yyvs,yyvsa)+\
		    __ALLOCA_free(yyls,yylsa)+\
		   (num))
#else
#define __ALLOCA_return(num) \
            return( __ALLOCA_free(yyss,yyssa)+\
		    __ALLOCA_free(yyvs,yyvsa)+\
		   (num))
#endif
#else
#define __ALLOCA_return(num) return(num)
#define __ALLOCA_alloca(size) alloca(size)
#define __ALLOCA_free(ptr,ref) 
#endif

/* ENDALLOCA SIMULATION */

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (YY_parse_CHAR = YYEMPTY)
#define YYEMPTY         -2
#define YYEOF           0
#define YYACCEPT        __ALLOCA_return(0)
#define YYABORT         __ALLOCA_return(1)
#define YYERROR         YYGOTO(yyerrlab1)
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL          YYGOTO(yyerrlab)
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do                                                              \
  if (YY_parse_CHAR == YYEMPTY && yylen == 1)                               \
    { YY_parse_CHAR = (token), YY_parse_LVAL = (value);                 \
      yychar1 = YYTRANSLATE (YY_parse_CHAR);                                \
      YYPOPSTACK;                                               \
      YYGOTO(yybackup);                                            \
    }                                                           \
  else                                                          \
    { YY_parse_ERROR ("syntax error: cannot back up"); YYERROR; }   \
while (0)

#define YYTERROR        1
#define YYERRCODE       256

#ifndef YY_parse_PURE
/* UNPURE */
#define YYLEX           YY_parse_LEX()
#ifndef YY_USE_CLASS
/* If nonreentrant, and not class , generate the variables here */
int     YY_parse_CHAR;                      /*  the lookahead symbol        */
YY_parse_STYPE      YY_parse_LVAL;              /*  the semantic value of the */
				/*  lookahead symbol    */
int YY_parse_NERRS;                 /*  number of parse errors so far */
#ifdef YY_parse_LSP_NEEDED
YY_parse_LTYPE YY_parse_LLOC;   /*  location data for the lookahead     */
			/*  symbol                              */
#endif
#endif


#else
/* PURE */
#ifdef YY_parse_LSP_NEEDED
#define YYLEX           YY_parse_LEX(&YY_parse_LVAL, &YY_parse_LLOC)
#else
#define YYLEX           YY_parse_LEX(&YY_parse_LVAL)
#endif
#endif
#ifndef YY_USE_CLASS
#if YY_parse_DEBUG != 0
int YY_parse_DEBUG_FLAG;                    /*  nonzero means print parse trace     */
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif
#endif



/*  YYINITDEPTH indicates the initial size of the parser's stacks       */

#ifndef YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif


#if __GNUC__ > 1                /* GNU C and GNU C++ define this.  */
#define __yy_bcopy(FROM,TO,COUNT)       __builtin_memcpy(TO,FROM,COUNT)
#else                           /* not GNU C or C++ */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */

#ifdef __cplusplus
static void __yy_bcopy (char *from, char *to, int count)
#else
#ifdef __STDC__
static void __yy_bcopy (char *from, char *to, int count)
#else
static void __yy_bcopy (from, to, count)
     char *from;
     char *to;
     int count;
#endif
#endif
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}
#endif

int
#ifdef YY_USE_CLASS
 YY_parse_CLASS::
#endif
     YY_parse_PARSE(YY_parse_PARSE_PARAM)
#ifndef __STDC__
#ifndef __cplusplus
#ifndef YY_USE_CLASS
/* parameter definition without protypes */
YY_parse_PARSE_PARAM_DEF
#endif
#endif
#endif
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YY_parse_STYPE *yyvsp;
  int yyerrstatus;      /*  number of tokens to shift before error messages enabled */
  int yychar1=0;          /*  lookahead token as an internal (translated) token number */

  short yyssa[YYINITDEPTH];     /*  the state stack                     */
  YY_parse_STYPE yyvsa[YYINITDEPTH];        /*  the semantic value stack            */

  short *yyss = yyssa;          /*  refer to the stacks thru separate pointers */
  YY_parse_STYPE *yyvs = yyvsa;     /*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YY_parse_LSP_NEEDED
  YY_parse_LTYPE yylsa[YYINITDEPTH];        /*  the location stack                  */
  YY_parse_LTYPE *yyls = yylsa;
  YY_parse_LTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;

#ifdef YY_parse_PURE
  int YY_parse_CHAR;
  YY_parse_STYPE YY_parse_LVAL;
  int YY_parse_NERRS;
#ifdef YY_parse_LSP_NEEDED
  YY_parse_LTYPE YY_parse_LLOC;
#endif
#endif

  YY_parse_STYPE yyval;             /*  the variable used to return         */
				/*  semantic values from the action     */
				/*  routines                            */

  int yylen;
/* start loop, in which YYGOTO may be used. */
YYBEGINGOTO

#if YY_parse_DEBUG != 0
  if (YY_parse_DEBUG_FLAG)
    fprintf(stderr, "Starting parse\n");
#endif
  yystate = 0;
  yyerrstatus = 0;
  YY_parse_NERRS = 0;
  YY_parse_CHAR = YYEMPTY;          /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YY_parse_LSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
YYLABEL(yynewstate)

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YY_parse_STYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YY_parse_LSP_NEEDED
      YY_parse_LTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YY_parse_LSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if yyoverflow is a macro.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yyls1, size * sizeof (*yylsp),
		 &yystacksize);
#else
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yystacksize);
#endif

      yyss = yyss1; yyvs = yyvs1;
#ifdef YY_parse_LSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  YY_parse_ERROR("parser stack overflow");
	  __ALLOCA_return(2);
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
      yyss = (short *) __ALLOCA_alloca (yystacksize * sizeof (*yyssp));
      __yy_bcopy ((char *)yyss1, (char *)yyss, size * sizeof (*yyssp));
      __ALLOCA_free(yyss1,yyssa);
      yyvs = (YY_parse_STYPE *) __ALLOCA_alloca (yystacksize * sizeof (*yyvsp));
      __yy_bcopy ((char *)yyvs1, (char *)yyvs, size * sizeof (*yyvsp));
      __ALLOCA_free(yyvs1,yyvsa);
#ifdef YY_parse_LSP_NEEDED
      yyls = (YY_parse_LTYPE *) __ALLOCA_alloca (yystacksize * sizeof (*yylsp));
      __yy_bcopy ((char *)yyls1, (char *)yyls, size * sizeof (*yylsp));
      __ALLOCA_free(yyls1,yylsa);
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YY_parse_LSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YY_parse_DEBUG != 0
      if (YY_parse_DEBUG_FLAG)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YY_parse_DEBUG != 0
  if (YY_parse_DEBUG_FLAG)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  YYGOTO(yybackup);
YYLABEL(yybackup)

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* YYLABEL(yyresume) */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    YYGOTO(yydefault);

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (YY_parse_CHAR == YYEMPTY)
    {
#if YY_parse_DEBUG != 0
      if (YY_parse_DEBUG_FLAG)
	fprintf(stderr, "Reading a token: ");
#endif
      YY_parse_CHAR = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (YY_parse_CHAR <= 0)           /* This means end of input. */
    {
      yychar1 = 0;
      YY_parse_CHAR = YYEOF;                /* Don't call YYLEX any more */

#if YY_parse_DEBUG != 0
      if (YY_parse_DEBUG_FLAG)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(YY_parse_CHAR);

#if YY_parse_DEBUG != 0
      if (YY_parse_DEBUG_FLAG)
	{
	  fprintf (stderr, "Next token is %d (%s", YY_parse_CHAR, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, YY_parse_CHAR, YY_parse_LVAL);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    YYGOTO(yydefault);

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	YYGOTO(yyerrlab);
      yyn = -yyn;
      YYGOTO(yyreduce);
    }
  else if (yyn == 0)
    YYGOTO(yyerrlab);

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YY_parse_DEBUG != 0
  if (YY_parse_DEBUG_FLAG)
    fprintf(stderr, "Shifting token %d (%s), ", YY_parse_CHAR, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (YY_parse_CHAR != YYEOF)
    YY_parse_CHAR = YYEMPTY;

  *++yyvsp = YY_parse_LVAL;
#ifdef YY_parse_LSP_NEEDED
  *++yylsp = YY_parse_LLOC;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  YYGOTO(yynewstate);

/* Do the default action for the current state.  */
YYLABEL(yydefault)

  yyn = yydefact[yystate];
  if (yyn == 0)
    YYGOTO(yyerrlab);

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
YYLABEL(yyreduce)
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YY_parse_DEBUG != 0
  if (YY_parse_DEBUG_FLAG)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


/* #line 811 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.cc" */
#line 1737 "kcparser.cpp"

  switch (yyn) {

case 75:
#line 175 "kcparser.y"
{yyval.node = MakeDeclaration(yyvsp[-1].node, nullptr);;
    break;}
case 76:
#line 176 "kcparser.y"
{yyval.node = MakeDeclaration(yyvsp[-2].node, nullptr);;
    break;}
case 77:
#line 180 "kcparser.y"
{yyval.node = MakeDeclSpec(yyvsp[0].node, nullptr);;
    break;}
case 78:
#line 181 "kcparser.y"
{yyval.node = MakeDeclSpec(yyvsp[-1].node, yyvsp[0].node);;
    break;}
case 79:
#line 182 "kcparser.y"
{yyval.node = MakeDeclSpec(yyvsp[0].node, nullptr);;
    break;}
case 80:
#line 183 "kcparser.y"
{yyval.node = MakeDeclSpec(yyvsp[-1].node, yyvsp[0].node);;
    break;}
case 81:
#line 184 "kcparser.y"
{yyval.node = MakeDeclSpec(yyvsp[0].node, nullptr);;
    break;}
case 82:
#line 185 "kcparser.y"
{yyval.node = MakeDeclSpec(yyvsp[-1].node, yyvsp[0].node);;
    break;}
case 87:
#line 199 "kcparser.y"
{yyval.node = MakeStorageClass(StorageClass::Typedef);;
    break;}
case 88:
#line 200 "kcparser.y"
{yyval.node = MakeStorageClass(StorageClass::Extern);;
    break;}
case 89:
#line 201 "kcparser.y"
{yyval.node = MakeStorageClass(StorageClass::Static);;
    break;}
case 90:
#line 202 "kcparser.y"
{yyval.node = MakeStorageClass(StorageClass::Auto);;
    break;}
case 91:
#line 203 "kcparser.y"
{yyval.node = MakeStorageClass(StorageClass::Register);;
    break;}
case 92:
#line 207 "kcparser.y"
{yyval.node = MakeTypeSpecifier(Type::Void);;
    break;}
case 93:
#line 208 "kcparser.y"
{yyval.node = MakeTypeSpecifier(Type::Char);;
    break;}
case 94:
#line 209 "kcparser.y"
{yyval.node = MakeTypeSpecifier(Type::Short);;
    break;}
case 95:
#line 210 "kcparser.y"
{yyval.node = MakeTypeSpecifier(Type::Int);;
    break;}
case 96:
#line 211 "kcparser.y"
{yyval.node = MakeTypeSpecifier(Type::Long);;
    break;}
case 97:
#line 212 "kcparser.y"
{yyval.node = MakeTypeSpecifier(Type::Float);;
    break;}
case 98:
#line 213 "kcparser.y"
{yyval.node = MakeTypeSpecifier(Type::Double);;
    break;}
case 99:
#line 214 "kcparser.y"
{yyval.node = MakeTypeSpecifier(Type::Signed);;
    break;}
case 100:
#line 215 "kcparser.y"
{yyval.node = MakeTypeSpecifier(Type::Unsigned);;
    break;}
case 101:
#line 216 "kcparser.y"
{yyval.node = nullptr;;
    break;}
case 102:
#line 217 "kcparser.y"
{yyval.node = nullptr;;
    break;}
case 103:
#line 218 "kcparser.y"
{yyval.node = nullptr;;
    break;}
case 128:
#line 276 "kcparser.y"
{yyval.node = MakeTypeQualifier(TypeQualifier::Const);;
    break;}
case 129:
#line 277 "kcparser.y"
{yyval.node = MakeTypeQualifier(TypeQualifier::Volatile);;
    break;}
case 204:
#line 422 "kcparser.y"
{ root = new KCNode("TranslationUnit", yyvsp[0].node); ;
    break;}
case 205:
#line 423 "kcparser.y"
{ root = new KCNode("TranslationUnit", yyvsp[-1].node, yyvsp[0].node); ;
    break;}
case 206:
#line 427 "kcparser.y"
{ yyval.node = new KCNode("ExternalDeclaration", yyvsp[0].node); ;
    break;}
case 207:
#line 428 "kcparser.y"
{ yyval.node = new KCNode("ExternalDeclaration", yyvsp[0].node); ;
    break;}
case 208:
#line 432 "kcparser.y"
{ yyval.node = new KCNode("FunctionDefinition"); ;
    break;}
case 209:
#line 433 "kcparser.y"
{ yyval.node = new KCNode("FunctionDefinition"); ;
    break;}
case 210:
#line 434 "kcparser.y"
{ yyval.node = new KCNode("FunctionDefinition"); ;
    break;}
case 211:
#line 435 "kcparser.y"
{ yyval.node = new KCNode("FunctionDefinition"); ;
    break;}
}

#line 811 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.cc"
   /* the action file gets copied in in place of this dollarsign  */
  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YY_parse_LSP_NEEDED
  yylsp -= yylen;
#endif

#if YY_parse_DEBUG != 0
  if (YY_parse_DEBUG_FLAG)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YY_parse_LSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = YY_parse_LLOC.first_line;
      yylsp->first_column = YY_parse_LLOC.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  YYGOTO(yynewstate);

YYLABEL(yyerrlab)   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++YY_parse_NERRS;

#ifdef YY_parse_ERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      YY_parse_ERROR(msg);
	      free(msg);
	    }
	  else
	    YY_parse_ERROR ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YY_parse_ERROR_VERBOSE */
	YY_parse_ERROR("parse error");
    }

  YYGOTO(yyerrlab1);
YYLABEL(yyerrlab1)   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (YY_parse_CHAR == YYEOF)
	YYABORT;

#if YY_parse_DEBUG != 0
      if (YY_parse_DEBUG_FLAG)
	fprintf(stderr, "Discarding token %d (%s).\n", YY_parse_CHAR, yytname[yychar1]);
#endif

      YY_parse_CHAR = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;              /* Each real token shifted decrements this */

  YYGOTO(yyerrhandle);

YYLABEL(yyerrdefault)  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) YYGOTO(yydefault);
#endif

YYLABEL(yyerrpop)   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YY_parse_LSP_NEEDED
  yylsp--;
#endif

#if YY_parse_DEBUG != 0
  if (YY_parse_DEBUG_FLAG)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

YYLABEL(yyerrhandle)

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    YYGOTO(yyerrdefault);

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    YYGOTO(yyerrdefault);

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	YYGOTO(yyerrpop);
      yyn = -yyn;
      YYGOTO(yyreduce);
    }
  else if (yyn == 0)
    YYGOTO(yyerrpop);

  if (yyn == YYFINAL)
    YYACCEPT;

#if YY_parse_DEBUG != 0
  if (YY_parse_DEBUG_FLAG)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = YY_parse_LVAL;
#ifdef YY_parse_LSP_NEEDED
  *++yylsp = YY_parse_LLOC;
#endif

  yystate = yyn;
  YYGOTO(yynewstate);
/* end loop, in which YYGOTO may be used. */
  YYENDGOTO
}

/* END */

/* #line 1010 "C:\\Users\\Filami\\Documents\\code\\vidf\\tools\\bison.cc" */
#line 2085 "kcparser.cpp"
#line 438 "kcparser.y"

#include <stdio.h>
#include "kcparser.h"

extern char yytext[];
extern int column;

void yyerror(char *s)
{
	fflush(stdout);
	printf("\n%*s\n%*s\n", column, "^", column, s);
}



void parse::yyerror(char* s)
{
	fflush(stdout);
	printf("\n%*s\n%*s\n", column, "^", column, s);
}
