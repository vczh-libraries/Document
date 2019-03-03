#include <Vlpp.h>

using namespace vl;
using namespace vl::filesystem;
using namespace vl::stream;

namespace operator_overloading
{
	struct Op
	{
		const wchar_t* name;
		bool parameter;

		Op(const wchar_t* _name, bool _parameter) :name(_name), parameter(_parameter) {}
	};

	Op ops[] = {
		{L"++",		true},
		{L"--",		true},
		{L"++",		false},
		{L"--",		false},
		{L"~",		false},
		{L"!",		false},
		{L"-",		false},
		{L"+",		false},
		{L"&",		false},
		{L"*",		false},
		{L"*",		true},
		{L"/",		true},
		{L"%",		true},
		{L"+",		true},
		{L"-",		true},
		{L"<<",		true},
		{L">>",		true},
		{L"==",		true},
		{L"!=",		true},
		{L"<",		true},
		{L"<=",		true},
		{L">",		true},
		{L">=",		true},
		{L"&",		true},
		{L"|",		true},
		{L"^",		true},
		{L"&&",		true},
		{L"||",		true},
		{L"=",		true},
		{L"*=",		true},
		{L"/=",		true},
		{L"%=",		true},
		{L"+=",		true},
		{L"-=",		true},
		{L"<<=",	true},
		{L">>=",	true},
		{L"&=",		true},
		{L"|=",		true},
		{L"^=",		true},
		{L",",		true},
	};

	void GenerateInput1()
	{
		FilePath path = L"../UnitTest/TestOverloadingOperator_Input1.h";
		FileStream fileStream(path.GetFullPath(), FileStream::WriteOnly);
		Utf8Encoder encoder;
		EncoderStream encoderStream(fileStream, encoder);
		StreamWriter writer(encoderStream);

		writer.WriteLine(L"namespace test_overloading");
		writer.WriteLine(L"{");
		writer.WriteLine(L"\tstruct X");
		writer.WriteLine(L"\t{");
		for (auto op : ops)
		{
			writer.WriteString(L"\t\tvoid* operator");
			writer.WriteString(op.name);
			writer.WriteString(op.name[1] ? L"\t" : L"\t\t");
			writer.WriteLine(op.parameter ? L"(int);" : L"();");
		}
		writer.WriteLine(L"");
		for (auto op : ops)
		{
			writer.WriteString(L"\t\tbool* operator");
			writer.WriteString(op.name);
			writer.WriteString(op.name[1] ? L"\t" : L"\t\t");
			writer.WriteLine(op.parameter ? L"(int)const;" : L"()const;");
		}
		writer.WriteLine(L"\t}");
		writer.WriteLine(L"}");
	}

	void GenerateInput2()
	{
	}

	void GenerateInput3()
	{
	}
}

int main()
{
	operator_overloading::GenerateInput1();
	operator_overloading::GenerateInput2();
	operator_overloading::GenerateInput3();
	return 0;
}