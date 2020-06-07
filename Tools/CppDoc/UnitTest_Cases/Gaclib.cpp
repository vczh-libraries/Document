// this is important, it removes hard-coded "#line <Specified-Number>" in VC++ STL source files
#define _DEBUG_FUNCTIONAL_MACHINERY

// this file is intended to include all these files.
// since the compiler only accept one preprocessed file.
// so even if your program has multiple cpp files,
// you need to create one cpp file to include all of them and then generate the preprocessed file.

#include "../../../../Vlpp/Release/IncludeOnly/Vlpp.cpp"
#include "../../../../VlppOS/Release/IncludeOnly/VlppOS.cpp"
#include "../../../../VlppRegex/Release/IncludeOnly/VlppRegex.cpp"
#include "../../../../VlppReflection/Release/IncludeOnly/VlppReflection.cpp"
#include "../../../../VlppParser/Release/IncludeOnly/VlppParser.cpp"
#include "../../../../Workflow/Release/IncludeOnly/VlppWorkflowLibrary.cpp"
#include "../../../../Workflow/Release/IncludeOnly/VlppWorkflowRuntime.cpp"
#include "../../../../Workflow/Release/IncludeOnly/VlppWorkflowCompiler.cpp"
#include "../../../../GacUI/Release/IncludeOnly/GacUI.cpp"
#include "../../../../GacUI/Release/IncludeOnly/GacUIWindows.cpp"
#include "../../../../GacUI/Release/IncludeOnly/GacUIReflection.cpp"
#include "../../../../GacUI/Release/IncludeOnly/GacUICompiler.cpp"
#include "../../../../GacUI/Release/IncludeOnly/DarkSkin.cpp"
#include "../../../../GacUI/Release/IncludeOnly/DarkSkinReflection.cpp"