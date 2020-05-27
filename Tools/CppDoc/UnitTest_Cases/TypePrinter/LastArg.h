#pragma once

namespace last_arg
{
	struct VoidType
	{
		void* tag = nullptr;
	};

	struct IntType
	{
		int tag = 0;
	};

	struct BoolType
	{
		bool tag = false;
	};

	VoidType LastArg() { return {}; }
	IntType LastArg(int) { return {}; }
	BoolType LastArg(bool) { return {}; }

	template<typename T>
	auto LastArg(T t) { return t; }

	template<typename T, typename... Ts>
	auto LastArg(T t, Ts... ts)
	{
		return LastArg(ts...);
	}
}