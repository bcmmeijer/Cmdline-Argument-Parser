#include <iostream>
#include <vector>
#include <string>
#include <any>
#include <unordered_map>
#include <sstream>
#include <filesystem>
#include <exception>

struct Flag {
	Flag(const char* s, const char* l, const char* d, const bool r, const char* x) :
		shortname(s), longname(l), description(d), defaults(x ? x : ""), required(r)
	{}

	bool		required;
	std::string shortname;
	std::string longname;
	std::string description;
	std::string defaults;
};

inline std::vector<Flag> _flags;
inline std::unordered_map<std::string, std::pair<bool, std::string>> _argmap;
inline bool _help_generated = false;
inline std::stringstream _help_required;
inline std::stringstream _help_optional;

class ArgNotFound : public std::exception {
public:
	ArgNotFound(const char* arg) {
		sprintf_s(data, "The required argument was not specified by the caller: %s", arg);
	}
	ArgNotFound(const std::string& arg) {
		sprintf_s(data, "The required argument was not specified by the caller: %s", arg.c_str());
	}
	virtual const char* what() const override {
		return data;
	}
private:
	char data[128];
};

class ArgParser {
public:

	ArgParser() = default;
	~ArgParser() = default;

	static void register_flag(const char* sname, const char* lname, const char* description, const bool is_required, const char* default_val = nullptr) noexcept {
		if (_has_been_registered(sname, lname)) return;
		_flags.emplace_back(sname, lname, description, is_required, default_val);
	}

	template <typename T>
	static T get_arg(const char* name) {

		const std::string* lname = _to_long(name);
		if (!lname) throw ArgNotFound(name);

		std::string value = _argmap[*lname].second;
		std::stringstream ss(value);
		
		T end;
		ss >> end;

		return end;
	}

	static void set_arg(const char* name, const char* value) {
		const std::string* lname = _to_long(name);
		if (!lname) throw ArgNotFound(name);
		_argmap[*lname].second = value;
	}

	static void parse_args(const int argc, char** argv) noexcept {
		
		for (int i = 1; i < argc;) {

			const std::string* lname = _to_long(argv[i]);
			if (!lname) {
				if ((strstr(argv[i + 1], "-") != nullptr)) i++;
				else i += 2;
				continue;
			}

			_argmap[*lname].first = true;

			if (i == argc - 1) {
				_argmap[*lname].second = "";
				i += 2;
			}
			else if ((strstr(argv[i + 1], "-") != nullptr)) {
				_argmap[*lname].second = "";
				i++;
			}
			else {
				_argmap[*lname].second = argv[i + 1];
				i += 2;
			}
		}

		for (const Flag& flag : _flags) {
			if (flag.required && _argmap.find(flag.longname) == _argmap.end())
				throw ArgNotFound(flag.longname);
		}
	}

	static void help() {
		if (!_help_generated) generate_help();

		std::cout << "Required args:\n";
		std::cout << _help_required.str();
		std::cout << "\nOptional args:\n";
		std::cout << _help_optional.str();
	}

	static void generate_help() {

		register_flag("-h", "--help", "Generates this message", false);

		std::stringstream* ss = nullptr;

		static auto get_max_boundaries = []() -> std::pair<size_t, size_t> {
			size_t names = 0, examples = 0;

			for (const Flag& flag : _flags) {
				size_t flagsize = flag.shortname.length() + flag.longname.length() + 1;
				size_t descsize = flag.defaults.length();

				names = std::max(names, flagsize);
				examples = std::max(examples, descsize);
			}
			return { names, examples };
		};

		auto [boundary_names, boundary_defaults] = get_max_boundaries();

		for (const Flag& flag : _flags) {

			if (flag.required) ss = &_help_required;
			else ss = &_help_optional;

			*ss << std::left << std::setw(boundary_names * 2) << (flag.shortname + "/" + flag.longname);
			*ss << std::left << std::setw(boundary_defaults * 2) << flag.defaults;
			*ss << flag.description << "\n";
		}

		_help_generated = true;
	}

private:
	static bool _has_been_registered(const char* s, const char* l) {
		for (const auto& flag : _flags) {
			if (flag.shortname == s || flag.longname == l)
				return true;
		}
		return false;
	}

	static const std::string* _to_long(const std::string& name) {
		if (name._Starts_with("--")) {
			for (const Flag& flag : _flags)
				if (flag.longname == name)
					return &flag.longname;
		}
		else {
			for (const Flag& flag : _flags)
				if (flag.shortname == name)
					return &flag.longname;
		}
		return nullptr;
	}

	static const Flag* _get_flag(const std::string& name) {
		for (const Flag& flag : _flags)
			if (flag.longname == name)
				return &flag;
		return nullptr;
	}
};