#include "Util.h"

#include <iostream>
#include <fstream>

namespace coral {

	std::string Util::read_file(const std::string& filename)
	{
		std::ifstream in(filename);

		if (!in.is_open()) {
			static const std::string msg = "Failed to open file: ";
			throw std::exception((msg + filename).c_str());
		}

		std::string str;

		in.seekg(0, std::ios::end);
		str.reserve(static_cast<size_t>(in.tellg()));
		in.seekg(0, std::ios::beg);

		str.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

		return str;
	}

	void Util::throw_exception(const std::string& message, const char* details)
	{
		std::string msg = message;
		msg += '\n';
		msg += details;
		throw std::exception(msg.c_str());
	}

	void Util::set_color(AnsiColor color)
	{
		const char* code = nullptr;
		switch (color) {
			case AnsiColor::RESET:
				code = "0";
				break;
			case AnsiColor::RED:
				code = "31;1";
				break;
			case AnsiColor::GREEN:
				code = "32;1";
				break;
			case AnsiColor::YELLOW:
				code = "33;1";
				break;
		}
		if (code != nullptr) {
			std::cout << "\u001b[" << code << 'm';
		}
	}

	void Util::clear_color()
	{
		set_color(AnsiColor::RESET);
	}

	void Util::print_divider(const char* text)
	{
		Util::set_color(AnsiColor::YELLOW);
		std::cout << "\n===== " << text << " =====\n\n";
		Util::clear_color();
	}

} // namespace coral
