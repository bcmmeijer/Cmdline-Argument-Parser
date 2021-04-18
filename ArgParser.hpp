#include <stdexcept>
#include <string>
#include <unordered_map>
#include <concepts>
#include <array>
#include <string_view>
#include <sstream>

template <typename T>
concept Lexible = requires { std::constructible_from<std::ostream&>; };

template <typename T>
concept Parsible = requires (T t) {
	{ T::Char };
	{ t.raw() };
};

template <typename Charset, bool Throwing>
class Argparserbase {
public:
	using Char = Charset;
	using String = std::basic_string<Charset>;
	using Strvw = std::basic_string_view<Charset>;
	using Stream = std::basic_stringstream<Charset>;

public:
	Argparserbase() = default;
	~Argparserbase() = default;

	Argparserbase(int argc, const Char** argv) {
		parse(argc, argv);
	}

	Argparserbase(Parsible auto& const other) {
		if constexpr (std::is_same_v<Char, std::decay_t<decltype(other)>::Char>) {
			this->_args = other.raw();
		}
		else {
			this->_args = convert_args(other.raw());
		}
	}

public:
	void parse(int argc, const Char** argv) {
		constexpr Char indicator = std::is_same_v<Char, char> ? '-' : L'-';

		for (int i = 1; i < argc; i++) {
			const Char* arg = argv[i];
			switch (arg[0]) {
			case indicator: _args[arg] = String();		break;
			default:		_args[argv[i - 1]] = arg;	break;
			}
		}
	}

	const auto& raw() const {
		return _args;
	}

	template <typename ... Args>
	bool has(Args ... args) const requires std::convertible_to<std::common_type_t<Args...>, Strvw> {
		return (hasimpl(args) || ...);
	}

	template <typename T, typename ... Args>
	T get(Args ... args) const requires std::convertible_to<std::common_type_t<Args...>, Strvw> {
		Strvw item = get(args...);
		if (item.empty()) return T{};
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

	template <typename T, typename ... Args>
	auto get_or(T def, Args ... args) const requires std::convertible_to<std::common_type_t<Args...>, Strvw> {
		Strvw item = get(args...);
		if constexpr (std::is_convertible_v<T, Strvw>)
			return item.empty() ? Strvw(def) : item;
		else
			return item.empty() ? def : lexical_cast<T>(item);
	}

private:
	bool hasimpl(Strvw str) const {
		return std::find_if(_args.begin(), _args.end(), [&str](auto& elem) {
			return str == elem.first.data();
		}) != _args.end();
	}

	const Strvw getimpl(Strvw str) const {
		return std::find_if(_args.begin(), _args.end(), [&str](auto& elem) {
			return str == elem.first.data();
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

	// arg to warg
	auto convert_args(const std::unordered_map<std::string, std::string>& args) const {
		static auto convert = [](const std::string& str) -> std::wstring {
			size_t size = str.size() + 1;
			std::wstring ret(size, L'\0');
			size_t out;

			mbstowcs_s(&out, ret.data(), size, str.data(), size - 1);
			ret.resize(out);
			return ret;
		};

		std::unordered_map<std::wstring, std::wstring> ret;

		for (auto& [k, v] : args)
			ret[convert(k)] = convert(v);

		return ret;
	}

	// warg to arg
	auto convert_args(const std::unordered_map<std::wstring, std::wstring>& wargs) const {
		static auto convert = [](const std::wstring& str) -> std::string {
			size_t size = str.size() + 1;
			std::string ret(size, '\0');
			size_t out;

			wcstombs_s(&out, ret.data(), size, str.data(), size - 1);
			ret.resize(out);
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

using Argparser = Argparserbase<char, false>;
using Wargparser = Argparserbase<wchar_t, false>;
using ThrowingArgparser = Argparserbase<char, true>;
using ThrowingWargparser = Argparserbase<wchar_t, true>;
