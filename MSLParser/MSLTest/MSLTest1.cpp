#include "stdafx.h"
#include "CppUnitTest.h"
#include "MSL/MSL.h"

using namespace std;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace MSLTest
{		
	TEST_CLASS(MSL_Parse_Float)
	{
	public:
		
		TEST_METHOD(Float_SimpleParse)
		{
			Assert::AreEqual(45.5f, msl::Value::fromString("45.5")->asFloat());
			Assert::AreEqual(-45.5f, msl::Value::fromString("-45.5")->asFloat());
			Assert::AreEqual(-1.f, msl::Value::fromString("-1")->asFloat());
		}

		TEST_METHOD(Float_PercentParse)
		{
			Assert::AreEqual(5.f, msl::Value::fromString("500%")->asFloat());
			Assert::AreEqual(0.5f, msl::Value::fromString("50%")->asFloat());
		}

	};

	TEST_CLASS(MSL_Parse_String)
	{
	public:

		TEST_METHOD(String_SimpleParse)
		{
			auto s = msl::Value::fromString("aaa.1");
			Assert::AreEqual("aaa.1"s, s->asString());

			auto s2 = msl::Value::fromString("\"aaa@1\"");
			Assert::AreEqual("aaa@1"s, s2->asString());

		}

		TEST_METHOD(String_Empty)
		{
			auto s = msl::Value::fromString("\" \"");
			Assert::AreEqual(" "s, s->asString());

			auto s2 = msl::Value::fromString("\"\"");
			Assert::AreEqual(""s, s2->asString());
		}

		TEST_METHOD(String_Multiline)
		{
			auto s = msl::Value::fromString("\"Line1\nLine2\nLine3\"");
			Assert::AreEqual("Line1\nLine2\nLine3"s, s->asString());
		}

		TEST_METHOD(String_Array)
		{
			auto a = msl::Value::fromString("[aaa bbb \"aaa bbb\"]");
			Assert::AreEqual(3, (int)a->asArray().size());
			Assert::AreEqual("aaa"s, a->asArray()[0]->asString());
			Assert::AreEqual("bbb"s, a->asArray()[1]->asString());
			Assert::AreEqual("aaa bbb"s, a->asArray()[2]->asString());
		}

	};

	TEST_CLASS(MSL_Parse_Array)
	{
	public:

		TEST_METHOD(Array_SimpleParse)
		{
			auto a = msl::Value::fromString("[1 2 3]");
			Assert::AreEqual(3, (int)a->asArray().size());

			Assert::AreEqual(1.f, a->asArray()[0]->asFloat());
			Assert::AreEqual(2.f, a->asArray()[1]->asFloat());
			Assert::AreEqual(3.f, a->asArray()[2]->asFloat());
		}

		TEST_METHOD(Array_Empty)
		{
			auto a = msl::Value::fromString("[ ]");
			Assert::AreEqual(0, (int)a->asArray().size());

			auto a2 = msl::Value::fromString("[]");
			Assert::AreEqual(0, (int)a2->asArray().size());
		}

		TEST_METHOD(Array_Array)
		{
			auto a = msl::Value::fromString("[[] [1] [1 2]]");
			Assert::AreEqual(3, (int)a->asArray().size());

			Assert::AreEqual(0, (int)a->asArray()[0]->asArray().size());
			Assert::AreEqual(1, (int)a->asArray()[1]->asArray().size());
			Assert::AreEqual(2, (int)a->asArray()[2]->asArray().size());
		}

	};

	TEST_CLASS(MSL_Parse_Comments)
	{
	public:

		TEST_METHOD(Comments_SimpleParse)
		{
			auto str = R"foo( 
			[
				0
				//1 [this is commented out
				2
				3
				/*4 also this ]*/
			]
			)foo";

			auto a = msl::Value::fromString(str);
			Assert::AreEqual(3, (int)a->asArray().size());

			Assert::AreEqual(0.f, a->asArray()[0]->asFloat());
			Assert::AreEqual(2.f, a->asArray()[1]->asFloat());
			Assert::AreEqual(3.f, a->asArray()[2]->asFloat());
		}



	};


}