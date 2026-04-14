/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_ARITH_ARITH_TAB_H_INCLUDED
# define YY_ARITH_ARITH_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef ARITH_DEBUG
# if defined YYDEBUG
#if YYDEBUG
#   define ARITH_DEBUG 1
#  else
#   define ARITH_DEBUG 0
#  endif
# else /* ! defined YYDEBUG */
#  define ARITH_DEBUG 0
# endif /* ! defined YYDEBUG */
#endif  /* ! defined ARITH_DEBUG */
#if ARITH_DEBUG
extern int arith_debug;
#endif

/* Token kinds.  */
#ifndef ARITH_TOKENTYPE
# define ARITH_TOKENTYPE
  enum arith_tokentype
  {
    ARITH_EMPTY = -2,
    ARITH_EOF = 0,                 /* "end of file"  */
    ARITH_error = 256,             /* error  */
    ARITH_UNDEF = 257,             /* "invalid token"  */
    NUMBER = 258,                  /* NUMBER  */
    PLUS = 259,                    /* PLUS  */
    MINUS = 260,                   /* MINUS  */
    STAR = 261,                    /* STAR  */
    SLASH = 262,                   /* SLASH  */
    LPAREN = 263,                  /* LPAREN  */
    RPAREN = 264                   /* RPAREN  */
  };
  typedef enum arith_tokentype arith_token_kind_t;
#endif

/* Value type.  */
#if ! defined ARITH_STYPE && ! defined ARITH_STYPE_IS_DECLARED
typedef int ARITH_STYPE;
# define ARITH_STYPE_IS_TRIVIAL 1
# define ARITH_STYPE_IS_DECLARED 1
#endif


extern ARITH_STYPE arith_lval;


int arith_parse (void);


#endif /* !YY_ARITH_ARITH_TAB_H_INCLUDED  */
