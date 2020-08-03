#pragma once

#include <functional>
#include <string>

namespace coral {

	enum class AnsiColor {
		RESET,
		YELLOW,
		RED,
		GREEN,
	};

	class Util {
	public:

		/// Read an entire file into a string.
		[[nodiscard]] static std::string read_file(const std::string& filename);

		/// Throw an exception with the given message.
		static void throw_exception(const std::string& message, const char* details);

		static void set_color(AnsiColor color);

		static void clear_color();

		static void print_divider(const char* text);

	};

} // namespace coral
