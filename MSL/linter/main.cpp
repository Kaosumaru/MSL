#include <iostream>
#include <fstream>
#include "msl/MSL.h"
#include "msl/Serializer.h"

void lintMSL(const char* in_path, const char* out_path)
{
	std::cout << "Trying to lint " << in_path << "\n";

	msl::Value::pointer v;
	{
		std::ifstream t(in_path);
		std::string str((std::istreambuf_iterator<char>(t)),
			std::istreambuf_iterator<char>());

		v = msl::Value::fromString(str);
	}

	std::ofstream out;
	out.open(out_path, std::ios::out | std::ios::trunc);
	msl::Serializer::Write(v, out);
}


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
	auto in = "v:/test/mech.json";
	auto out = "v:/test/mech.msl";
	lintMSL(in, out);
#endif



	return 0;
}