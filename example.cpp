#include "ArgParser.hpp"

int main(int argc, char** argv) {

	ArgParser::register_flag("-d", "--delay", "specifies delay", true, "10ms");
	ArgParser::register_flag("-i", "--iterations", "Specifies iterations of loop", false, "20");

	ArgParser::generate_help();
	ArgParser::help();

	try {

		ArgParser::parse_args(argc, argv);

		auto delay = ArgParser::get_arg<int>("-d");
		auto iterations = ArgParser::get_arg<int>("-i");
	}
	catch (const std::exception& e) {
		std::cout << e.what() << std::endl;
	}

	return 0;
}