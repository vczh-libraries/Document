#include "ExportReflectableTypes.h"
#include <GacUICompiler.h>

using namespace vl::collections;
using namespace vl::stream;
using namespace vl::parsing::xml;
using namespace vl::reflection::description;

namespace vl
{
	namespace reflection
	{
		namespace description
		{
			extern bool LoadGuiBasicTypes();
			extern bool LoadGuiElementTypes();
			extern bool LoadGuiCompositionTypes();
			extern bool LoadGuiEventTypes();
			extern bool LoadGuiTemplateTypes();
			extern bool LoadGuiControlTypes();
		}
	}
}

void LogTypes(Ptr<XmlElement> xmlRoot, const WString& projectName, SortedList<WString>& loaded)
{
	auto xmlProject = MakePtr<XmlElement>();
	xmlProject->name.value = L"project";
	xmlRoot->subNodes.Add(xmlProject);
	{
		auto attr = MakePtr<XmlAttribute>();
		attr->name.value = L"name";
		attr->value.value = projectName;
	}
}

void ExportReflectableTypes(Folder outputPath)
{
	FilePath pathReflection = outputPath.GetFilePath() / L"Reflection.xml";

	auto xmlRoot = MakePtr<XmlElement>();
	xmlRoot->name.value = L"reflection";

	auto xmlDocument = MakePtr<XmlDocument>();
	xmlDocument->rootElement = xmlRoot;

	SortedList<WString> loaded;
	{
		LoadPredefinedTypes();
		GetGlobalTypeManager()->Load();
		LogTypes(xmlRoot, L"VlppReflection", loaded);
		ResetGlobalTypeManager();
	}
	{
		LoadPredefinedTypes();
		LoadParsingTypes();
		XmlLoadTypes();
		JsonLoadTypes();
		GetGlobalTypeManager()->Load();
		LogTypes(xmlRoot, L"VlppParser", loaded);
		ResetGlobalTypeManager();
	}
	{
		LoadPredefinedTypes();
		WfLoadLibraryTypes();
		GetGlobalTypeManager()->Load();
		LogTypes(xmlRoot, L"Workflow-Runtime", loaded);
		ResetGlobalTypeManager();
	}
	{
		LoadPredefinedTypes();
		LoadParsingTypes();
		WfLoadTypes();
		GetGlobalTypeManager()->Load();
		LogTypes(xmlRoot, L"Workflow-Compiler", loaded);
		ResetGlobalTypeManager();
	}
	{
		LoadPredefinedTypes();
		LoadParsingTypes();
		XmlLoadTypes();
		JsonLoadTypes();
		WfLoadLibraryTypes();
		LoadGuiBasicTypes();
		LoadGuiElementTypes();
		LoadGuiCompositionTypes();
		LoadGuiEventTypes();
		LoadGuiTemplateTypes();
		LoadGuiControlTypes();
		GetGlobalTypeManager()->Load();
		LogTypes(xmlRoot, L"GacUI", loaded);
		ResetGlobalTypeManager();
	}
	{
		LoadPredefinedTypes();
		LoadParsingTypes();
		GuiIqLoadTypes();
		GetGlobalTypeManager()->Load();
		LogTypes(xmlRoot, L"GacUI-Compiler", loaded);
		ResetGlobalTypeManager();
	}

	{
		FileStream fileStream(pathReflection.GetFullPath(), FileStream::WriteOnly);
		Utf8Encoder encoder;
		EncoderStream encoderStream(fileStream, encoder);
		StreamWriter writer(encoderStream);
		XmlPrint(xmlDocument, writer);
	}
}

void GuiMain()
{
}