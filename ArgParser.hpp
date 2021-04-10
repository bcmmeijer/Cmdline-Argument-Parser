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
class Argparserbase;

using Argparser = Argparserbase<char, false>;
using Wargparser = Argparserbase<wchar_t, false>;
using ThrowingArgparser = Argparserbase<char, true>;
using ThrowingWargparser = Argparserbase<wchar_t, true>;

template <typename Charset, bool Throwing>
class Argparserbase {
public:
	using Char = Charset;
	using CharPtr = Char*;
	using String = std::basic_string<Charset>;
	using Strvw = std::basic_string_view<Charset>;
	using Stream = std::basic_stringstream<Charset>;

public:
	Argparserbase() = default;
	~Argparserbase() = default;

	Argparserbase(ThrowingArgparser& other) {
		if constexpr (std::is_same_v<Char, ThrowingArgparser::Char>) {
			this->_args = other.raw();
		}
		else {
			this->_args = arg_to_warg(other.raw());
		}
	}

	Argparserbase(ThrowingWargparser& other) {
		if constexpr (std::is_same_v<Char, ThrowingWargparser::Char>) {
			this->_args = other.raw();
		}
		else {
			this->_args = warg_to_arg(other.raw());
		}
	}

	Argparserbase(Argparser& other) {
		if constexpr (std::is_same_v<Char, Argparser::Char>) {
			this->_args = other.raw();
		}
		else {
			this->_args = arg_to_warg(other.raw());
		}
	}

	Argparserbase(Wargparser& other) {
		if constexpr (std::is_same_v<Char, Wargparser::Char>) {
			this->_args = other.raw();
		}
		else {
			this->_args = warg_to_arg(other.raw());
		}
	}

public:
	void parse(int argc, CharPtr* argv) {
		constexpr Char indicator = std::is_same_v<Char, char> ? '-' : L'-';

		for (int i = 1; i < argc; i++) {
			CharPtr arg = argv[i];
			switch (arg[0]) {
			case indicator: _args[arg] = String();		break;
			default:		_args[argv[i - 1]] = arg;	break;
			}
		}
	}

	const auto raw() const {
		return _args;
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

	auto arg_to_warg(auto& args) {
		static auto convert = [](const std::string& str) -> std::wstring {
			size_t size = str.size() + 1;
			std::wstring ret(size, L'\0');
			size_t out;

			mbstowcs_s(&out, ret.data(), size, str.data(), size - 1);
			return ret;
		};

		std::unordered_map<std::wstring, std::wstring> ret;

		for (auto& [k, v] : args)
			ret[convert(k)] = convert(v);

		return ret;
	}

	auto warg_to_arg(auto& wargs) {
		static auto convert = [](const std::wstring& str) -> std::string {
			size_t size = str.size() + 1;
			std::string ret(size, '\0');
			size_t out;

			wcstombs_s(&out, ret.data(), size, str.data(), size - 1);
			return ret;
		};

		std::unordered_map<std::string, std::string> ret;

		for (auto& [k, v] : wargs)
			ret[convert(k)] = convert(v);

		return ret;
	}

private:
	std::unordered_map<String, String> _args;
};
