#ifndef VCZH_DOCUMENT_CPPDOC_LEXER
#define VCZH_DOCUMENT_CPPDOC_LEXER

#include "Vlpp.h"

using namespace vl;
using namespace vl::collections;
using namespace vl::regex;
using namespace vl::stream;
using namespace vl::filesystem;

/***********************************************************************
Definition
***********************************************************************/

enum class CppTokens
{
	LBRACE, RBRACE,
	LBRACKET, RBRACKET,
	LPARENTHESIS, RPARENTHESIS,
	LT, GT, EQ, NOT,
	PERCENT, COLON, SEMICOLON, DOT, QUESTIONMARK, COMMA,
	MUL, ADD, SUB, DIV, XOR, AND, OR, REVERT, SHARP,
	INT, HEX, BIN, FLOAT,
	STRING, CHAR,
	OPERATOR, NEW, DELETE, CONSTEXPR, CONST, VOLATILE, OVERRIDE, NOEXCEPT, THROW, DECLTYPE,
	__CDECL, __CLRCALL, __STDCALL, __FASTCALL, __THISCALL, __VECTORCALL,
	TYPE_AUTO, TYPE_VOID, TYPE_BOOL, TYPE_CHAR, TYPE_WCHAR_T, TYPE_CHAR16_T, TYPE_CHAR32_T,
	TYPE_SHORT, TYPE_INT, TYPE___INT8, TYPE___INT16, TYPE___INT32, TYPE___INT64, TYPE_LONG, TYPE_FLOAT, TYPE_DOUBLE,
	SIGNED, UNSIGNED,
	ID,
	SPACE, DOCUMENT, COMMENT1, COMMENT2,
};

extern Ptr<RegexLexer> CreateCppLexer();

/***********************************************************************
Reader
***********************************************************************/

class CppTokenCursor;
class CppTokenReader;

class CppTokenCursor
{
	friend class CppTokenReader;
private:
	CppTokenReader*				reader;
	Ptr<CppTokenCursor>			next;

	CppTokenCursor(CppTokenReader* _reader, RegexToken _token);
public:
	RegexToken					token;

	Ptr<CppTokenCursor>			Next();
};

class CppTokenReader : public Object
{
	friend class CppTokenCursor;
protected:
	Ptr<RegexLexer>				lexer;
	Ptr<RegexTokens>			tokens;
	IEnumerator<RegexToken>*	tokenEnumerator;

	Ptr<CppTokenCursor>			firstToken;

	Ptr<CppTokenCursor>			CreateNextToken();
public:
	CppTokenReader(Ptr<RegexLexer> _lexer, const WString& input);
	~CppTokenReader();

	Ptr<CppTokenCursor>			GetFirstToken();
};

#endif