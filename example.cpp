// EXAMPLE
// CLI ARGS: "-i 10 -o file.txt -q"

#include "ArgParser.hpp"
#include <iostream>
int wmain(int argc, wchar_t** argv) {

	ThrowingWargparser parser;
	parser.parse(argc, argv);

	auto count = parser.get<int>(L"-i");
	auto name = parser.get(L"-o");
	bool quiet = parser.has(L"-q");

	try {
		auto doesnotexist = parser.get(L"--doesnotexist");	// throws
	}
	catch (std::runtime_error& e) {
		std::cout << e.what() << "\n";
	}

	std::wcout << L"count: " << count << L"\n";
	std::wcout << L"name:  " << name << L"\n";
	std::wcout << L"quiet: " << quiet << L"\n";

	return 0;
}
