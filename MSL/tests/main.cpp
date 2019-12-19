#include <iostream>
#include <stdexcept>
#include <vector>

#ifndef _MSC_VER
#define lest_FEATURE_COLOURISE 1
#endif

#include "lest.hpp"
#include "msl/MSL.h"


using namespace std;

auto vmsl(const std::string& str)
{
	return msl::Value::fromString(str);
}

bool equals(float v, const msl::Value::pointer& ptr)
{
	return v == ptr->asFloat();
}

bool equals(const std::string& v, const msl::Value::pointer& ptr)
{
	return v == ptr->asString();
}

template<typename T>
bool equals(const std::vector<T>& v, const msl::Value::pointer& ptr)
{
	int i = 0;
	for (auto& e : v)
	{
		auto& mentry = ptr->asArray()[i];
		if (!equals(e, mentry)) return false;
		i++;
	}
	return true;
}

template<typename T>
bool is_equal(const T& t, const char* str)
{
	auto m = vmsl(str);
	if (!m) return false;
	return equals(t, m);
}

const lest::test specification[] =
{
	CASE("Float")
	{
		EXPECT(is_equal(45.5f, "45.5"));
		EXPECT(is_equal(-45.5f, "-45.5"));
		EXPECT(is_equal(-1.f, "-1"));
	},
	CASE("Percent")
	{
		//EXPECT(5.f == msl::Value::fromString("500%")->asFloat());
		//EXPECT(0.5f == msl::Value::fromString("50%")->asFloat());
	},
	CASE("String")
	{
		EXPECT(is_equal("aaa.1", "aaa.1"));
		EXPECT(is_equal("&<aaa>.1", "&<aaa>.1"));
	},
	CASE("Array_Float")
	{
		std::vector<float> v = { 1,2,3 };
		EXPECT(is_equal(v, "[1 2 3]"));
		EXPECT(is_equal(v, "[1, 2, 3]"));
		EXPECT(is_equal(v, "[1, 2, 3,]"));
	},

};



int main (int argc, char * argv[])
{
	return lest::run(specification, argc, argv);
}
