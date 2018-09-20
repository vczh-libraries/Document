#include "Lexer.h"

Ptr<RegexLexer> CreateCppLexer()
{
	List<WString> tokens;
#define DEFINE_TOKEN(NAME, REGEX) \
	do if ((vint)CppTokens::NAME != tokens.Add(REGEX)) { throw 0; } while(false)

	DEFINE_TOKEN(LBRACE, L"/{");
	DEFINE_TOKEN(RBRACE, L"/}");
	DEFINE_TOKEN(LBRACKET, L"/[");
	DEFINE_TOKEN(RRACKET, L"/]");
	DEFINE_TOKEN(LPARENTHESIS, L"/(");
	DEFINE_TOKEN(RPARENTHESIS, L"/)");
	DEFINE_TOKEN(LT, L"<");
	DEFINE_TOKEN(GT, L">");
	DEFINE_TOKEN(EQ, L"=");
	DEFINE_TOKEN(NOT, L"!");
	DEFINE_TOKEN(PERCENT, L"%");
	DEFINE_TOKEN(COLON, L":");
	DEFINE_TOKEN(SEMICOLON, L";");
	DEFINE_TOKEN(DOT, L".");
	DEFINE_TOKEN(QUESTIONMARK, L"/?");
	DEFINE_TOKEN(COMMA, L",");
	DEFINE_TOKEN(MUL, L"/*");
	DEFINE_TOKEN(ADD, L"/+");
	DEFINE_TOKEN(SUB, L"-");
	DEFINE_TOKEN(DIV, L"//");
	DEFINE_TOKEN(XOR, L"/^");
	DEFINE_TOKEN(AND, L"&");
	DEFINE_TOKEN(OR, L"/|");
	DEFINE_TOKEN(REVERT, L"~");

	DEFINE_TOKEN(INT, L"(/d+('/d+)*)[uU]?[lL]?");
	DEFINE_TOKEN(HEX, L"0[xX][0-9a-fA-F][uU]?[lL]?");
	DEFINE_TOKEN(BIN, L"0[bB][01]+[uU]?[lL]?");
	DEFINE_TOKEN(FLOAT, L"(/d+.|./d+|/d+./d+)([eE][+/-]?/d+)?[fFlL]?");
	DEFINE_TOKEN(STRING, L"([uUL]|u8)?\"([^/\\\"]|/\\/.)*\"");
	DEFINE_TOKEN(CHAR, L"([uUL]|u8)?'([^/\\']|/\\/.)*'");

	DEFINE_TOKEN(SPACE, L"/s+");
	DEFINE_TOKEN(DOCUMENT, L"//////[^\n]*");
	DEFINE_TOKEN(COMMENT1, L"////[^\n]*");
	DEFINE_TOKEN(COMMENT2, L"///*([^*]|/*+[^*//])*/*//");

#undef DEFINE_TOKEN

	RegexProc proc;
	return new RegexLexer(tokens, proc);
}