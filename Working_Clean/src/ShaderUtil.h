#pragma once

#include <glew/glew.h>

#include <exception>
#include <string>

namespace coral {

	class ShaderCompilationException : std::exception {};

	class ShaderUtil {
	public:

		[[nodiscard]] static GLuint compile_shader(const std::string& vertex_shader_path, const std::string& fragment_shader_path, const char* label);

	};

} // namespace coral

