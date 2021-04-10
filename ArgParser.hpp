#include <stdexcept>
#include <string>
#include <unordered_map>
#include <concepts>
#include <array>
#include <string_view>
#include <sstream>

template <typename T>
concept Lexible = requires { std::constructible_from<std::ostream&>; };

template <typename Charset, bool Throwing>
class Argparserbase {
private:
	using Char = Charset;
	using CharPtr = Char*;
	using String = std::basic_string<Charset>;
	using Strvw = std::basic_string_view<Charset>;
	using Stream = std::basic_stringstream<Charset>;

public:
	Argparserbase() = default;
	~Argparserbase() = default;

	void parse(int argc, CharPtr* argv) {
		constexpr Char indicator = std::is_same_v<Char, char> ? '-' : L'-';

		for (int i = 1; i < argc; i++) {
			CharPtr arg = argv[i];
			switch (arg[0]) {
			case indicator: _args[arg] = String();	break;
			default:  	_args[argv[i - 1]] = arg;	break;
			}
		}
	}

	template <typename ... Args>
	bool has(Args ... args) const requires std::convertible_to<std::common_type_t<Args...>, Strvw> {
		return (hasimpl(args) || ...);
	}

	template <typename T, typename ... Args>
	T get(Args ... args) const requires std::convertible_to<std::common_type_t<Args...>, Strvw> {
		Strvw item = get(args...);
		return lexical_cast<T>(item);
	}

	template <typename ... Args>
	Strvw get(Args ... args) const requires std::convertible_to<std::common_type_t<Args...>, Strvw> {
		const std::array<Strvw, sizeof ... (args)> items = { args... };

		for (Strvw item : items) {
			if (hasimpl(item)) { 
				return getimpl(item); 
			};
		}

		if constexpr (Throwing)
			throw std::runtime_error("Argument not specified");
		else
			return Strvw();
	}

private:
	bool hasimpl(Strvw str) const {
		return std::find_if(_args.begin(), _args.end(), [&str](auto& elem) {
			return elem.first == str;
		}) != _args.end();
	}

	const Strvw getimpl(Strvw str) const {
		return std::find_if(_args.begin(), _args.end(), [&str](auto& elem) {
			return elem.first == str;
		})->second;
	}

	template <Lexible T>
	T lexical_cast(Strvw str) const {
		Stream s;
		T res;

		s << str;
		s >> res;

		return res;
	}

private:
	std::unordered_map<String, String> _args;
};

using Argparser = Argparserbase<char, false>;
using Wargparser = Argparserbase<wchar_t, false>;
using ThrowingArgparser = Argparserbase<char, true>;
using ThrowingWargparser = Argparserbase<wchar_t, true>;
