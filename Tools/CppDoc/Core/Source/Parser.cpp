#include "Parser.h"

/***********************************************************************
Symbol
***********************************************************************/

void Symbol::Add(Ptr< Symbol> child)
{
	child->parent = this;
	children.Add(child->name, child);
}