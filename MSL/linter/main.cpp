#include <iostream>
#include <fstream>
#include "msl/MSL.h"
#include "msl/Serializer.h"


int main(int argc, char* argv[])
{
#ifndef _DEBUG
	if (argc == 1)
	{
		std::cout << "No input file\n";
		return 1;
	}

	auto path = argv[1];
#else
	auto path = "v:/test/stats.msl";
	auto res = "v:/test/stats2.msl";
#endif

	std::cout << "Trying to convert " << path << "\n";


	std::ifstream t(path);
	std::string str((std::istreambuf_iterator<char>(t)),
		std::istreambuf_iterator<char>());

	auto v = msl::Value::fromString(str);

	std::ofstream out;
	out.open(res, std::ios::out | std::ios::trunc);
	msl::Serializer::Write(v, out);

	return 0;
}