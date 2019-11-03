
%{
#include <stdio.h>
#include "kcparser.h"

void count(const char* yytext);
void comment();
int check_type();

%}


%define CONSTRUCTOR_PARAM FILE* in
%define CONSTRUCTOR_CODE  yyin = in; LineNo = 1;
%define MEMBERS int LineNo;
// %define LEX_PARAM YY_parse_STYPE& yylval


D			[0-9]
L			[a-zA-Z_]
H			[a-fA-F0-9]
E			[Ee][+-]?{D}+
FS			(f|F|l|L)
IS			(u|U|l|L)*



%%
"/*"			{ comment(); }

"auto"			{ count(yytext); return(parse::AUTO); }
"break"			{ count(yytext); return(parse::BREAK); }
"case"			{ count(yytext); return(parse::CASE); }
"char"			{ count(yytext); return(parse::CHAR); }
"const"			{ count(yytext); return(parse::CONST); }
"continue"		{ count(yytext); return(parse::CONTINUE); }
"default"		{ count(yytext); return(parse::DEFAULT); }
"do"			{ count(yytext); return(parse::DO); }
"double"		{ count(yytext); return(parse::DOUBLE); }
"else"			{ count(yytext); return(parse::ELSE); }
"enum"			{ count(yytext); return(parse::ENUM); }
"extern"		{ count(yytext); return(parse::EXTERN); }
"float"			{ count(yytext); return(parse::FLOAT); }
"for"			{ count(yytext); return(parse::FOR); }
"goto"			{ count(yytext); return(parse::GOTO); }
"if"			{ count(yytext); return(parse::IF); }
"int"			{ count(yytext); return(parse::INT); }
"long"			{ count(yytext); return(parse::LONG); }
"register"		{ count(yytext); return(parse::REGISTER); }
"return"		{ count(yytext); return(parse::RETURN); }
"short"			{ count(yytext); return(parse::SHORT); }
"signed"		{ count(yytext); return(parse::SIGNED); }
"sizeof"		{ count(yytext); return(parse::SIZEOF); }
"static"		{ count(yytext); return(parse::STATIC); }
"struct"		{ count(yytext); return(parse::STRUCT); }
"switch"		{ count(yytext); return(parse::SWITCH); }
"typedef"		{ count(yytext); return(parse::TYPEDEF); }
"union"			{ count(yytext); return(parse::UNION); }
"unsigned"		{ count(yytext); return(parse::UNSIGNED); }
"void"			{ count(yytext); return(parse::VOID); }
"volatile"		{ count(yytext); return(parse::VOLATILE); }
"while"			{ count(yytext); return(parse::WHILE); }

{L}({L}|{D})*		{ count(yytext); return(check_type()); }

0[xX]{H}+{IS}?		{ count(yytext); return(parse::CONSTANT); }
0{D}+{IS}?		{ count(yytext); return(parse::CONSTANT); }
{D}+{IS}?		{ count(yytext); return(parse::CONSTANT); }
L?'(\\.|[^\\'])+'	{ count(yytext); return(parse::CONSTANT); }

{D}+{E}{FS}?		{ count(yytext); return(parse::CONSTANT); }
{D}*"."{D}+({E})?{FS}?	{ count(yytext); return(parse::CONSTANT); }
{D}+"."{D}*({E})?{FS}?	{ count(yytext); return(parse::CONSTANT); }

L?\"(\\.|[^\\"])*\"	{ count(yytext); return(parse::STRING_LITERAL); }

"..."			{ count(yytext); return(parse::ELLIPSIS); }
">>="			{ count(yytext); return(parse::RIGHT_ASSIGN); }
"<<="			{ count(yytext); return(parse::LEFT_ASSIGN); }
"+="			{ count(yytext); return(parse::ADD_ASSIGN); }
"-="			{ count(yytext); return(parse::SUB_ASSIGN); }
"*="			{ count(yytext); return(parse::MUL_ASSIGN); }
"/="			{ count(yytext); return(parse::DIV_ASSIGN); }
"%="			{ count(yytext); return(parse::MOD_ASSIGN); }
"&="			{ count(yytext); return(parse::AND_ASSIGN); }
"^="			{ count(yytext); return(parse::XOR_ASSIGN); }
"|="			{ count(yytext); return(parse::OR_ASSIGN); }
">>"			{ count(yytext); return(parse::RIGHT_OP); }
"<<"			{ count(yytext); return(parse::LEFT_OP); }
"++"			{ count(yytext); return(parse::INC_OP); }
"--"			{ count(yytext); return(parse::DEC_OP); }
"->"			{ count(yytext); return(parse::PTR_OP); }
"&&"			{ count(yytext); return(parse::AND_OP); }
"||"			{ count(yytext); return(parse::OR_OP); }
"<="			{ count(yytext); return(parse::LE_OP); }
">="			{ count(yytext); return(parse::GE_OP); }
"=="			{ count(yytext); return(parse::EQ_OP); }
"!="			{ count(yytext); return(parse::NE_OP); }
";"			{ count(yytext); return(';'); }
("{"|"<%")		{ count(yytext); return('{'); }
("}"|"%>")		{ count(yytext); return('}'); }
","			{ count(yytext); return(','); }
":"			{ count(yytext); return(':'); }
"="			{ count(yytext); return('='); }
"("			{ count(yytext); return('('); }
")"			{ count(yytext); return(')'); }
("["|"<:")		{ count(yytext); return('['); }
("]"|":>")		{ count(yytext); return(']'); }
"."			{ count(yytext); return('.'); }
"&"			{ count(yytext); return('&'); }
"!"			{ count(yytext); return('!'); }
"~"			{ count(yytext); return('~'); }
"-"			{ count(yytext); return('-'); }
"+"			{ count(yytext); return('+'); }
"*"			{ count(yytext); return('*'); }
"/"			{ count(yytext); return('/'); }
"%"			{ count(yytext); return('%'); }
"<"			{ count(yytext); return('<'); }
">"			{ count(yytext); return('>'); }
"^"			{ count(yytext); return('^'); }
"|"			{ count(yytext); return('|'); }
"?"			{ count(yytext); return('?'); }

[ \t\v\n\f]		{ count(yytext); }
.			{ /* ignore bad characters */ }

%%

int yywrap()
{
	return 1;
}


void comment()
{
/*
	char c, c1;

loop:
	while ((c = input()) != '*' && c != 0)
		putchar(c);

	if ((c1 = input()) != '/' && c != 0)
	{
		unput(c1);
		goto loop;
	}

	if (c != 0)
		putchar(c1);
	*/
}


int column = 0;


void count(const char* yytext)
{
	int i;

	for (i = 0; yytext[i] != '\0'; i++)
		if (yytext[i] == '\n')
			column = 0;
		else if (yytext[i] == '\t')
			column += 8 - (column % 8);
		else
			column++;
}


int check_type()
{
	return(parse::IDENTIFIER);
}
