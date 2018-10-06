#ifndef VCZH_DOCUMENT_CPPDOC_LEXER
#define VCZH_DOCUMENT_CPPDOC_LEXER

#include <Vlpp.h>
#include "LexerTokenDef.h"

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
#define DEFINE_TOKEN(NAME, SOMETHING) NAME,
	CPP_ALL_TOKENS(DEFINE_TOKEN, DEFINE_TOKEN)
#undef DEFINE_TOKEN
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