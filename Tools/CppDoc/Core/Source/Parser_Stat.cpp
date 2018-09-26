#include "Parser.h"
#include "Ast_Stat.h"

Ptr<Stat> ParseStat(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	if (TestToken(cursor, CppTokens::LBRACE))
	{
		RequireToken(cursor, CppTokens::RBRACE);
		return MakePtr<BlockStat>();
	}
	throw StopParsingException(cursor);
}