#ifndef VCZH_DOCUMENT_CPPDOC_LEXER
#define VCZH_DOCUMENT_CPPDOC_LEXER

#include <VlppRegex.h>
#include <VlppOS.h>
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
	void						Clone(Ptr<CppTokenReader>& _reader, Ptr<CppTokenCursor>& _cursor);
};

class CppTokenReader : public Object
{
	friend class CppTokenCursor;
protected:
	bool						gotFirstToken = false;
	bool						skipSpaceAndComment = false;
	Ptr<RegexLexer>				lexer;
	Ptr<RegexTokens>			tokens;
	IEnumerator<RegexToken>*	tokenEnumerator;

	Ptr<CppTokenCursor>			CreateNextToken();

	CppTokenReader(const CppTokenReader& _reader, bool _skipSpaceAndComment);
public:
	CppTokenReader(Ptr<RegexLexer> _lexer, const WString& input, bool _skipSpaceAndComment = true);
	~CppTokenReader();

	Ptr<CppTokenCursor>			GetFirstToken();
};

#endif