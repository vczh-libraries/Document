#include "ExportReflectableTypes.h"
#include <GacUICompiler.h>

using namespace vl::collections;
using namespace vl::stream;
using namespace vl::parsing::xml;

void ExportReflectableTypes(Folder outputPath)
{
	FilePath pathReflection = outputPath.GetFilePath() / L"Reflection.xml";

	auto xmlRoot = MakePtr<XmlElement>();
	xmlRoot->name.value = L"Reflection";

	auto xmlDocument = MakePtr<XmlDocument>();
	xmlDocument->rootElement = xmlRoot;

	{
		FileStream fileStream(pathReflection.GetFullPath(), FileStream::WriteOnly);
		Utf8Encoder encoder;
		EncoderStream encoderStream(fileStream, encoder);
		StreamWriter writer(encoderStream);
		XmlPrint(xmlDocument, writer);
	}
}