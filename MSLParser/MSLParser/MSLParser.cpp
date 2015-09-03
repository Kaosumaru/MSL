// MSLParser.cpp : Defines the entry point for the console application.
//


#include "stdafx.h"
#include "MSL/MSL.h"





/*
MSL Parser

Wbudowane typy:
- string (cudzys?owia, bez cudzys?owiu)
- float (1. 1 100%)
- boolean (true, false)
- null

- tablica [10 20 30]
- mapa {1:1 2:2 3:3}

- named value   name() value

-komentarze


[name() aaa 
*/





int main()
{
	R"foo(

	{
		HP: 100
		Damage: 20
	
	}

	)foo";



#if 1

#ifdef TST_1
	{
		std::string str = "34.1";
		auto p = client::parse_msl(str.begin(), str.end());

		std::string strArr = R"foo(

		[
			Test.1
			34.1
			{
				1:1 
				"Mateusz":"Borycki"
			}
			"Test"
		]

		)foo";

		auto arr = client::parse_msl(strArr.begin(), strArr.end());
	}
#endif
	
	{
		std::string strArr = R"foo(

		[
			50%
			tr()[1 2 3]
			/*"Test"*/
			"Test2"
			//"Test3"
		]

		)foo";

		auto arr = msl::Value::fromString(strArr);
	}


#else
	{
		std::string str = "34.1";
		auto p = client::parse_msl(str.begin(), str.end());
	}

	{
		std::string str = "aaa sadas";
		auto p = client::parse_complex(str.begin(), str.end());
	}

	{
		std::string str = "\"aaa sa\"das";
		auto p = client::parse_complex(str.begin(), str.end());
	}
#endif

    return 0;
}

