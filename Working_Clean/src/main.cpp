#include "Util.h"

#include <sdl/SDL.h>
#include <glew/glew.h>

#include <iostream>
#include <functional>

using namespace coral;

static void GLAPIENTRY gl_message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	bool is_error = type == GL_DEBUG_TYPE_ERROR;
	std::ostream& output = (is_error ? std::cerr : std::cout);

	output << "GL Callback: " << (is_error ? "** GL Error **" : "Non Error") << " - Severity: " << severity << '\n';
	output << message << '\n';
}

namespace {

	void print_div(const char* text)
	{
		Util::set_color(AnsiColor::YELLOW);
		std::cout << "\n===== " << text << " =====\n\n";
		Util::clear_color();
	}

	struct Vector3 {
		GLfloat x;
		GLfloat y;
		GLfloat z;
	};

}

class Program {

	static constexpr unsigned int SWAP_DELAY = 1000u / 60u + 1;

	SDL_Window* window = nullptr;
	SDL_GLContext context = nullptr;

	GLuint program = 0u;
	GLuint vao = 0u;
	GLuint vbo_vertices = 0u;

	bool quit = false;
	bool skip_render = false;
	SDL_Event cur_event{};

	static constexpr Vector3 vertices[] = {
		{ -0.5f, +0.5f, 0.5f },
		{ -0.5f, -0.5f, 0.5f },
		{ +0.5f, +0.5f, 0.5f },
		{ +0.5f, -0.5f, 0.5f },
	};

	enum UniformLocation {
		UNIFORM_WINDOW_SIZE = 0,
		UNIFORM_TIME = 1,
		UNIFORM_MOUSE_POSITION = 2,
	};

public:

	Program()
	{
		window = SDL_CreateWindow("SDL + OpenGL",
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720,
			SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
		if (window == nullptr) {
			throw std::exception(SDL_GetError());
		}

		static constexpr int CHANNEL_SIZE = 8;
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, CHANNEL_SIZE);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, CHANNEL_SIZE);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, CHANNEL_SIZE);
		SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, CHANNEL_SIZE);
		SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, CHANNEL_SIZE * 4);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

		context = SDL_GL_CreateContext(window);
		if (context == nullptr) {
			throw std::exception(SDL_GetError());
		}

		if (glewInit() != GLEW_OK) {
			throw std::exception("GLEW failed to init");
		}

		glEnable(GL_DEBUG_OUTPUT);
		glDebugMessageCallback(gl_message_callback, nullptr);

		// Force update to trigger viewport resize and setting uniforms
		SDL_SetWindowSize(window, 1280, 720);
	}

	~Program()
	{
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo_vertices);
		glDeleteProgram(program);

		SDL_GL_DeleteContext(context);
		SDL_DestroyWindow(window);
	}

	void run()
	{
		//log_info();

		create_shader();
		create_buffer();

		glPointSize(4.0f);
		glPolygonMode(GL_BACK, GL_LINE);

		while (!quit) {
			handle_events();

			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			if (!skip_render) {
				glUseProgram(program);
				update_uniforms();

				glBindVertexArray(vao);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
				glBindVertexArray(NULL);

				glUseProgram(NULL);
			}

			SDL_GL_SwapWindow(window);

			SDL_Delay(SWAP_DELAY);
		}
	}

private:

	void update_uniforms()
	{
		glUniform1f(UNIFORM_TIME, static_cast<float>(SDL_GetTicks()) / 1000.0f);
		int x, y;
		SDL_GetMouseState(&x, &y);
		int height;
		SDL_GetWindowSize(window, nullptr, &height);
		glUniform2i(UNIFORM_MOUSE_POSITION, x, height - y);
	}

	void log_info()
	{
		print_div("INFO BEGIN");

		std::cout << glGetString(GL_VERSION) << '\n';
		std::cout << glGetString(GL_VENDOR) << '\n';
		std::cout << glGetString(GL_RENDERER) << '\n';
		std::cout << glGetString(GL_SHADING_LANGUAGE_VERSION) << '\n';

		GLint max;
		glGetIntegerv(GL_MAX_LABEL_LENGTH, &max);
		std::cout << "Max Label Length: " << max << '\n';

#if 0
		// Log supported GLSL versions
		{
			GLint num;
			glGetIntegerv(GL_NUM_SHADING_LANGUAGE_VERSIONS, &num);
			for (int i = 0; i < num; ++i) {
				std::cout << glGetStringi(GL_SHADING_LANGUAGE_VERSION, i) << '\n';
			}
		}
#endif

		print_div("INFO END");
	}

	void create_shader()
	{
		print_div("SHADER BEGIN");

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

		std::string place = "../Working_Clean/src/";

		GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
		create(place + "first.vert", vertex_shader);

		GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
		create(place + "first.frag", frag_shader);

		program = glCreateProgram();
		glObjectLabel(GL_PROGRAM, program, -1, "agfx::program");
		glAttachShader(program, vertex_shader);
		glAttachShader(program, frag_shader);

		glLinkProgram(program);
		glGetProgramInfoLog(program, sizeof(out_buf), nullptr, out_buf);

		std::cout << "Program Info:\n" << out_buf;

		glDetachShader(program, vertex_shader);
		glDeleteShader(vertex_shader);

		glDetachShader(program, frag_shader);
		glDeleteShader(frag_shader);

		GLint success;
		glGetProgramiv(program, GL_LINK_STATUS, &success);
		if (!success) {
			Util::set_color(AnsiColor::RED);
			puts("Shader did not link successfully. Skipping render loop.");
			Util::clear_color();
		} else {
			skip_render = false;

			Sint32 width, height;
			SDL_GetWindowSize(window, &width, &height);
			glUseProgram(program);
			glUniform2i(0, width, height);
			glUseProgram(NULL);
		}

		print_div("SHADER END");

	}

	void create_buffer()
	{
		// VAO
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glObjectLabel(GL_VERTEX_ARRAY, vao, -1, "agfx::vao");

		// VBO
		glGenBuffers(1, &vbo_vertices);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
		glObjectLabel(GL_BUFFER, vbo_vertices, -1, "agfx::vbo_vertices");

		glBufferStorage(GL_ARRAY_BUFFER, sizeof(vertices), NULL, GL_MAP_WRITE_BIT);
		void* map_ptr = glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(vertices), GL_MAP_WRITE_BIT);
		std::memcpy(map_ptr, vertices, sizeof(vertices));
		glUnmapBuffer(GL_ARRAY_BUFFER);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), reinterpret_cast<void*>(0));

		

		// Cleanup
		glBindBuffer(GL_ARRAY_BUFFER, NULL);
		glBindVertexArray(NULL);
	}

	void handle_events()
	{
		while (SDL_PollEvent(&cur_event)) {
			switch (cur_event.type) {

				case SDL_QUIT:
					quit = true;
					break;

				case SDL_KEYDOWN:
					switch (cur_event.key.keysym.scancode) {

						case SDL_SCANCODE_F4:
							quit = true;
							break;

						case SDL_SCANCODE_R:
							Util::set_color(AnsiColor::GREEN);
							puts("Hot reloading shaders...");
							Util::clear_color();

							glDeleteProgram(program);
							create_shader();
							break;
					}
					break;

				case SDL_WINDOWEVENT:
					switch (cur_event.window.event) {
						case SDL_WINDOWEVENT_SIZE_CHANGED:
							Sint32 width = cur_event.window.data1;
							Sint32 height = cur_event.window.data2;

							Util::set_color(AnsiColor::GREEN);
							std::cout << "Window resized to " << width << " x " << height << '\n';
							Util::clear_color();

							glViewport(0, 0, width, height);
							glUseProgram(program);
							glUniform2i(UNIFORM_WINDOW_SIZE, width, height);
							glUseProgram(NULL);

							break;
					}
					break;

			}
		}
	}

};

int main(int, char**)
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		std::cerr << "SDL Failed to init\n";
	}

	try {
		Program().run();
	} catch (std::exception& e) {
		std::cerr << "Program panicked:\n" << e.what() << '\n';
		SDL_Quit();
		return 1;
	}

	SDL_Quit();
	return 0;
}