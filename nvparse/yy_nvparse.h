#ifndef _YY_NVPARSE_H
#define _YY_NVPARSE_H

/*
  this hack allows several yacc parseer to be 
  linked in the same binary on UNIX platforms
*/

#define	yymaxdepth nvparse_maxdepth
#define	yyparse	   nvparse_parse
#define	yylex      nvparse_lex
#define	yyerror	   nvparse_error
#define	yylval     nvparse_lval
#define	yychar     nvparse_char
#define	yydebug	   nvparse_debug
#define	yypact	   nvparse_pact
#define	yyr1	   nvparse_r1
#define	yyr2	   nvparse_r2
#define	yydef	   nvparse_def
#define	yychk	   nvparse_chk
#define	yypgo	   nvparse_pgo
#define	yyact	   nvparse_act
#define	yyexca	   nvparse_exca
#define yyerrflag  nvparse_errflag
#define yynerrs	   nvparse_nerrs
#define	yyps	   nvparse_ps
#define	yypv	   nvparse_pv
#define	yys	   nvparse_s
#define	yy_yys	   nvparse_yys
#define	yystate	   nvparse_state
#define	yytmp	   nvparse_tmp
#define	yyv	   nvparse_v
#define	yy_yyv	   nvparse_yyv
#define	yyval	   nvparse_val
#define	yylloc	   nvparse_lloc
#define yyreds	   nvparse_reds
#define yytoks	   nvparse_toks
#define yylhs	   nvparse_yylhs
#define yylen	   nvparse_yylen
#define yydefred   nvparse_yydefred
#define yydgoto	   nvparse_yydgoto
#define yysindex   nvparse_yysindex
#define yyrindex   nvparse_yyrindex
#define yygindex   nvparse_yygindex
#define yytable	   nvparse_yytable
#define yycheck	   nvparse_yycheck
#define yyname     nvparse_yyname
#define yyrule     nvparse_yyrule
#define yyin       nvparse_yyin
#define yyout      nvparse_yyout
#define yywrap     nvparse_yywrap

#endif
