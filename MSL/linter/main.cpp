#include <iostream>
#include <fstream>
#include <filesystem>
#include "msl/MSL.h"
#include "msl/Serializer.h"

void lintMSL(const char* in_path)
{
	std::filesystem::path out_path{ in_path };
	out_path = out_path.parent_path() / out_path.stem();
	out_path += "_linted.msl";
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
	lintMSL(path);
#else
	auto in = "v:/test/mech.json";
	lintMSL(in);
#endif



	return 0;
}