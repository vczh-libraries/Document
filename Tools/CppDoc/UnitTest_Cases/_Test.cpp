// this is important, it removes hard-coded "#line <Specified-Number>" in VC++ STL source files
#define _DEBUG_FUNCTIONAL_MACHINERY

// this file is intended to include all these files.
// since the compiler only accept one preprocessed file.
// so even if your program has multiple cpp files,
// you need to create one cpp file to include all of them and then generate the preprocessed file.

#include "../../../../VlppParser/Release/IncludeOnly/VlppParser.h"