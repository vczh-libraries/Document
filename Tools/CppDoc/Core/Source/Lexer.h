#ifndef VCZH_DOCUMENT_CPPDOC_LEXER
#define VCZH_DOCUMENT_CPPDOC_LEXER

#include "Vlpp.h"

using namespace vl;
using namespace vl::collections;
using namespace vl::regex;

enum class CppTokens
{
	LBRACE, RBRACE,
	LBRACKET, RRACKET,
	LPARENTHESIS, RPARENTHESIS,
	LT, GT, EQ, NOT,
	PERCENT, COLON, SEMICOLON, DOT, QUESTIONMARK, COMMA,
	MUL, ADD, SUB, DIV, XOR, AND, OR, REVERT,
	INT, HEX, BIN, FLOAT,
	STRING, CHAR,
	SPACE, DOCUMENT, COMMENT1, COMMENT2,
};

extern Ptr<RegexLexer> CreateCppLexer();

#endif