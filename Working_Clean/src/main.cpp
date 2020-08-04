#include "Util.h"

#include "ShaderUtil.h"

#include <sdl/SDL.h>
#include <glew/glew.h>

#include <array>
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

	struct Vector3 {
		GLfloat x;
		GLfloat y;
		GLfloat z;
	};

	struct Color {
		GLfloat r;
		GLfloat g;
		GLfloat b;
		GLfloat a;
	};

}

class VertexStream {

public:

	// ==============
	// Exported Types
	// ==============

	struct Vertex {
		Vector3 position;
		Color color;
	};


private:

	// ==============
	// Representation
	// ==============

	GLuint vao = 0u;
	GLuint vbo_vertices = 0u;

	const std::size_t VERTEX_COUNT;

public:

	// ============
	// Construction
	// ============

	template<std::size_t ArrSize>
	VertexStream(const std::array<Vertex, ArrSize>& vertices)
		: VERTEX_COUNT(ArrSize)
	{
		// Create VAO
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glObjectLabel(GL_VERTEX_ARRAY, vao, -1, "agfx::VAO");

		// Create vertices buffer
		glGenBuffers(1, &vbo_vertices);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
		glObjectLabel(GL_BUFFER, vbo_vertices, -1, "agfx::VAO.Vertices");

		glBufferStorage(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(), 0u);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, color)));

		glBindBuffer(GL_ARRAY_BUFFER, NULL);

		// Cleanup
		glBindVertexArray(NULL);
	}

	~VertexStream()
	{
		glDeleteBuffers(1, &vbo_vertices);
		glDeleteVertexArrays(1, &vao);
	}

	VertexStream(const VertexStream&) = delete;
	VertexStream& operator=(const VertexStream&) = delete;

	VertexStream(VertexStream&&) = default;
	VertexStream& operator=(VertexStream&&) = default;

	// ================
	// Public Interface
	// ================

	void draw() const noexcept
	{
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, VERTEX_COUNT);
		glBindVertexArray(NULL);
	}

};

class Program {

	static constexpr unsigned int SWAP_DELAY = 1000u / 60u + 1;

	float total_time = 0.0f;
	float corrected_time = 0.0f;

	SDL_Window* window = nullptr;
	SDL_GLContext context = nullptr;

	GLuint program = 0u;
	GLuint ubo_transform = 0u;
	GLuint ubo_time = 0u;
	std::unique_ptr<VertexStream> vertex_stream{};

	bool quit = false;
	bool skip_render = false;
	SDL_Event cur_event{};

	const Uint8* ScancodeMap = nullptr;

	static constexpr std::array<VertexStream::Vertex, 4> VERTICES = {
		VertexStream::Vertex{ // Top Left
			Vector3{ -1.0f, +1.0f, 0.5f },
			Color{1.0f, 0.0f, 0.0f, 1.0f},
		},
		VertexStream::Vertex{ // Bottom Left
			Vector3{ -1.0f, -1.0f, 0.5f },
			Color{1.0f, 0.0f, 1.0f, 1.0f},
		},
		VertexStream::Vertex{ // Top Right
			Vector3{ +1.0f, +1.0f, 0.5f },
			Color{1.0f, 1.0f, 0.0f, 1.0f},
		},
		VertexStream::Vertex{ // Bottom Right
			Vector3{ +1.0f, -1.0f, 0.5f },
			Color{1.0f, 1.0f, 1.0f, 1.0f},
		},
	};

	enum UniformLocation {
		UNIFORM_WINDOW_SIZE = 0,
		UNIFORM_TIME = 1,
		UNIFORM_MOUSE_POSITION = 2,
	};

	struct UniformBlockBinding {
		enum {
			TIME = 0,
			TRANFORM = 1,
		};
	};

	struct UniformBlockSize {
		static constexpr unsigned int TIME = 4 * 2;
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
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE); // no notifications

		ScancodeMap = SDL_GetKeyboardState(nullptr);

		// Force update to trigger viewport resize and setting uniforms
		SDL_SetWindowSize(window, 1280, 720);
	}

	~Program()
	{
		glDeleteBuffers(1, &ubo_time);
		glDeleteProgram(program);

		SDL_GL_DeleteContext(context);
		SDL_DestroyWindow(window);
	}

	void run() noexcept
	{
		create_buffers();
		create_shader();
		vertex_stream.reset(new VertexStream(VERTICES));

		glPointSize(4.0f);
		glPolygonMode(GL_BACK, GL_LINE);

		while (!quit) {
			handle_events();

			total_time += SWAP_DELAY / 1000.0f;
			if (!ScancodeMap[SDL_SCANCODE_RSHIFT]) {
				corrected_time += SWAP_DELAY / 1000.0f;
			}

			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			if (!skip_render) {
				glUseProgram(program);
				update_uniforms();

				vertex_stream->draw();

				glUseProgram(NULL);
			}

			SDL_GL_SwapWindow(window);

			SDL_Delay(SWAP_DELAY);
		}
	}

private:

	void update_ubo_time() noexcept
	{
		glBindBuffer(GL_UNIFORM_BUFFER, ubo_time);
		std::byte* ptr = static_cast<std::byte*>(glMapBufferRange(GL_UNIFORM_BUFFER, 0, UniformBlockSize::TIME, GL_MAP_WRITE_BIT));
		new(ptr) float{ total_time };
		ptr += 4;
		new(ptr) float{ corrected_time };
		glUnmapBuffer(GL_UNIFORM_BUFFER);
		glBindBuffer(GL_UNIFORM_BUFFER, NULL);
	}

	/// Shader program must be active ahead of time
	void update_uniforms() noexcept
	{
		// UBOs
		update_ubo_time();

		// Program specific
		glUniform1f(UNIFORM_TIME, static_cast<float>(SDL_GetTicks()) / 1000.0f);
		int x, y;
		SDL_GetMouseState(&x, &y);
		int height;
		SDL_GetWindowSize(window, nullptr, &height);
		glUniform2i(UNIFORM_MOUSE_POSITION, x, height - y);
	}

	void log_info() noexcept
	{
		Util::print_divider("Info Begin");

		std::cout << glGetString(GL_VERSION) << '\n';
		std::cout << glGetString(GL_VENDOR) << '\n';
		std::cout << glGetString(GL_RENDERER) << '\n';
		std::cout << glGetString(GL_SHADING_LANGUAGE_VERSION) << '\n';
		std::puts("");

		GLint value;
		glGetIntegerv(GL_MAX_LABEL_LENGTH, &value);
		std::cout << "Max label length: " << value << '\n';

		glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &value);
		std::cout << "Max vertex attributes: " << value << '\n';

		glGetIntegerv(GL_MAX_UNIFORM_LOCATIONS, &value);
		std::cout << "Max uniform locations: " << value << '\n';

		glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &value);
		std::cout << "Max uniform buffers: " << value << '\n';

		glGetIntegerv(GL_MAX_VERTEX_UNIFORM_BLOCKS, &value);
		std::cout << "Max uniform buffers in vertex shader: " << value << '\n';

		glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_BLOCKS, &value);
		std::cout << "Max uniform buffers in fragment shader: " << value << '\n';

		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &value);
		std::cout << "Max texture size: " << value << '\n';
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

		Util::print_divider("Info End");
	}

	void create_buffers() noexcept
	{
		glGenBuffers(1, &ubo_time);
		glBindBuffer(GL_UNIFORM_BUFFER, ubo_time);
		glObjectLabel(GL_BUFFER, ubo_time, -1, "agfx::UBO::TimeBlock");
		glBufferStorage(GL_UNIFORM_BUFFER, UniformBlockSize::TIME, nullptr, GL_MAP_WRITE_BIT);
		glBindBuffer(GL_UNIFORM_BUFFER, NULL);

		glBindBufferBase(GL_UNIFORM_BUFFER, UniformBlockBinding::TIME, ubo_time);
	}

	void create_shader() noexcept
	{
		static const std::string BASE_PATH = "../Working_Clean/src/";

		Util::print_divider("Shader Compilation Begin");

		try {
			program = ShaderUtil::compile_shader(
				BASE_PATH + "first.vert",
				BASE_PATH + "first.frag");

			std::puts("Shader compiled successfully");

			// Set window size uniform
			glUseProgram(program);
			Sint32 width, height;
			SDL_GetWindowSize(window, &width, &height);
			glUniform2i(0, width, height);
			glUseProgram(NULL);

			// Setup uniform blocks
			if (const GLuint idx = glGetUniformBlockIndex(program, "TransformBlock"); idx == GL_INVALID_INDEX) {
				std::cerr << "Failed to find index for transform block\n";
			} else {
				glUniformBlockBinding(program, idx, UniformBlockBinding::TRANFORM);
				//glBindBufferBase(GL_UNIFORM_BUFFER, UniformBlockBinding::TRANFORM, ub_transform);
			}

			glBindBufferBase(GL_UNIFORM_BUFFER, UniformBlockBinding::TIME, ubo_time);

			skip_render = false;
		} catch (const ShaderCompilationException&) {
			program = NULL;

			Util::set_color(AnsiColor::RED);
			std::puts("Shader failed to compile");
			Util::clear_color();

			skip_render = true;
		}

		Util::print_divider("Shader Compilation End");
	}

	void handle_events() noexcept
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

						case SDL_SCANCODE_F1:
							log_info();
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
							if (program != NULL) {
								glUseProgram(program);
								glUniform2i(UNIFORM_WINDOW_SIZE, width, height);
								glUseProgram(NULL);
							}

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