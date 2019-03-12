#pragma once

#include "Expr.h"

namespace calculator
{
	extern Expr::Ptr			Parse(const char* input);
}