#include "ShaderUtil.h"

#include "Util.h"

#include <sdl\SDL_video.h>

#include <iostream>

namespace coral {

	GLuint ShaderUtil::compile_shader(const std::string& vertex_shader_path, const std::string& fragment_shader_path)
	{
		GLchar out_buf[1024];
		const GLchar* src[1];
		GLint length[1];

		auto create = [&](const std::string& name, GLuint handle) {
			std::string source = Util::read_file(name);

			src[0] = source.data();
			length[0] = source.length();

			glShaderSource(handle, 1, src, length);
			glCompileShader(handle);
		};

		GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
		create(vertex_shader_path, vertex_shader);

		GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
		create(fragment_shader_path, frag_shader);

		GLuint program = glCreateProgram();
		glObjectLabel(GL_PROGRAM, program, -1, "agfx::ShaderProgram");
		glAttachShader(program, vertex_shader);
		glAttachShader(program, frag_shader);

		glLinkProgram(program);
		glGetProgramInfoLog(program, sizeof(out_buf), nullptr, out_buf);

		// Log info if not empty
		if (out_buf[0] != '\0') {
			std::cout << "Program Info:\n" << out_buf;
		}

		glDetachShader(program, vertex_shader);
		glDeleteShader(vertex_shader);

		glDetachShader(program, frag_shader);
		glDeleteShader(frag_shader);

		GLint success;
		glGetProgramiv(program, GL_LINK_STATUS, &success);
		glUseProgram(NULL);

		if (!success) {
			glDeleteProgram(program);
			throw ShaderCompilationException{};
		} else {
			return program;
		}
	}

} // namespace coral