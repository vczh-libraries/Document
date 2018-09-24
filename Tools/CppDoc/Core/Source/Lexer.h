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

#define CPP_KEYWORD_TOKENS(F)\
	F(OPERATOR,			operator)\
	F(NEW,				new)\
	F(DELETE,			delete)\
	F(CONSTEXPR,		constexpr)\
	F(CONST,			const)\
	F(VOLATILE,			volatile)\
	F(OVERRIDE,			override)\
	F(NOEXCEPT,			noexcept)\
	F(THROW,			throw)\
	F(DECLTYPE,			decltype)\
	F(__CDECL,			__cdecl)\
	F(__CLRCALL,		__clrcall)\
	F(__STDCALL,		__stdcall)\
	F(__FASTCALL,		__fastcall)\
	F(__THISCALL,		__thiscall)\
	F(__VECTORCALL,		__vectorcall)\
	F(TYPE_AUTO,		auto)\
	F(TYPE_VOID,		void)\
	F(TYPE_BOOL,		bool)\
	F(TYPE_CHAR,		char)\
	F(TYPE_WCHAR_T,		wchar_t)\
	F(TYPE_CHAR16_T,	char16_t)\
	F(TYPE_CHAR32_T,	char32_t)\
	F(TYPE_SHORT,		short)\
	F(TYPE_INT,			int)\
	F(TYPE___INT8,		__int8)\
	F(TYPE___INT16,		__int16)\
	F(TYPE___INT32,		__int32)\
	F(TYPE___INT64,		__int64)\
	F(TYPE_LONG,		long)\
	F(TYPE_FLOAT,		float)\
	F(TYPE_DOUBLE,		double)\
	F(SIGNED,			signed)\
	F(UNSIGNED,			unsigned)\

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
#define DEFINE_KEYWORD_TOKEN(NAME, KEYWORD) NAME,
	CPP_KEYWORD_TOKENS(DEFINE_KEYWORD_TOKEN)
#undef DEFINE_KEYWORD_TOKEN
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