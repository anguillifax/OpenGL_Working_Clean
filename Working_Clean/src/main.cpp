#include "Util.h"

#include "ShaderUtil.h"
#include "Model.h"

#include <glew/glew.h>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>
#include <sdl/SDL.h>

#include <array>
#include <cstdint>
#include <functional>
#include <iostream>

using namespace coral;

static void GLAPIENTRY gl_message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	bool is_error = type == GL_DEBUG_TYPE_ERROR;
	std::ostream& output = (is_error ? std::cerr : std::cout);

	output << "GL Callback: " << (is_error ? "** GL Error **" : "Non Error") << " - Severity: " << severity << '\n';
	output << message << '\n';
}

class UniformBlockApplication {

	static constexpr unsigned int BINDING = 0u;
	static constexpr unsigned int SIZE =
		4 * 2   // window_size
		+ 4 * 2 // mouse_position
		+ 4     // total_time
		+ 4     // corrected_time
		;

	GLuint ubo;

public:

	UniformBlockApplication()
	{
		glGenBuffers(1, &ubo);
		glBindBuffer(GL_UNIFORM_BUFFER, ubo);
		glObjectLabel(GL_BUFFER, ubo, -1, "UBO::Application");
		glBufferStorage(GL_UNIFORM_BUFFER, SIZE, nullptr, GL_MAP_WRITE_BIT);
		glBindBuffer(GL_UNIFORM_BUFFER, NULL);
	}

	~UniformBlockApplication()
	{
		glDeleteBuffers(1, &ubo);
	}

	UniformBlockApplication(const UniformBlockApplication&) = delete;
	UniformBlockApplication& operator=(const UniformBlockApplication&) = delete;

	UniformBlockApplication(UniformBlockApplication&&) = default;
	UniformBlockApplication& operator=(UniformBlockApplication&&) = default;

	void bind() const
	{
		glBindBufferBase(GL_UNIFORM_BUFFER, BINDING, ubo);
	}

	void update(int win_width, int win_height, int mouse_x, int mouse_y, float ttime, float ctime)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, ubo);
		std::byte* ptr = static_cast<std::byte*>(glMapBufferRange(GL_UNIFORM_BUFFER, 0, SIZE, GL_MAP_WRITE_BIT));

		new(ptr) int(win_width);
		new(ptr += 4) int(win_height);

		new(ptr += 4) int(mouse_x);
		new(ptr += 4) int(mouse_y);

		new(ptr += 4) float(ttime);
		new(ptr += 4) float(ctime);

		glUnmapBuffer(GL_UNIFORM_BUFFER);
		glBindBuffer(GL_UNIFORM_BUFFER, NULL);
	}

};

struct VertexBank {

	static constexpr std::array<Model::Vertex, 4> RECT = {
		Model::Vertex{ // Top Left
			glm::vec3{ -1.0f, +1.0f, 0.5f },
		},
		Model::Vertex{ // Bottom Left
			glm::vec3{ -1.0f, -1.0f, 0.5f },
		},
		Model::Vertex{ // Top Right
			glm::vec3{ +1.0f, +1.0f, 0.5f },
		},
		Model::Vertex{ // Bottom Right
			glm::vec3{ +1.0f, -1.0f, 0.5f },
		},
	};

};

class Program {

	static constexpr unsigned int SWAP_DELAY = 1000u / 60u + 1;

	float total_time = 0.0f;
	float corrected_time = 0.0f;

	SDL_Window* window = nullptr;
	SDL_GLContext context = nullptr;

	GLuint program = 0u;
	std::unique_ptr<UniformBlockApplication> ub_application{};
	std::unique_ptr<Model> model{};

	bool quit = false;
	bool skip_render = false;
	SDL_Event cur_event{};

	const Uint8* ScancodeMap = nullptr;

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

		create_buffers();
		create_shader();
		ub_application.reset(new UniformBlockApplication());
		model.reset(new Model(VertexBank::RECT.data(), VertexBank::RECT.size(), "Model::Main"));

		// Force update to trigger viewport resize
		SDL_SetWindowSize(window, 1280, 720);
	}

	~Program()
	{
		glDeleteProgram(program);

		SDL_GL_DeleteContext(context);
		SDL_DestroyWindow(window);
	}

	void run()
	{
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

			update_uniforms();

			if (!skip_render) {
				glUseProgram(program);
				ub_application->bind();

				model->draw();

				glUseProgram(NULL);
			}

			SDL_GL_SwapWindow(window);

			SDL_Delay(SWAP_DELAY);
		}
	}

private:


	void update_uniforms()
	{
		int mx, my;
		SDL_GetMouseState(&mx, &my);
		int width, height;
		SDL_GetWindowSize(window, &width, &height);

		ub_application->update(width, height, mx, height - my, total_time, corrected_time);
	}

	void log_info()
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

	void create_buffers()
	{
	}

	void create_shader()
	{
		static const std::string BASE_PATH = "../Working_Clean/shaders/";

		static const std::string FNAME = "first";

		Util::print_divider("Shader Compilation Begin");

		try {
			program = ShaderUtil::compile_shader(
				BASE_PATH + FNAME + ".vert",
				BASE_PATH + FNAME + ".frag",
				"Shader::Main");

			std::puts("Shader compiled successfully");

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

	void on_window_resize(signed int width, signed int height)
	{
		Util::set_color(AnsiColor::GREEN);
		std::cout << "Window resized to " << width << " x " << height << '\n';
		Util::clear_color();

		glViewport(0, 0, width, height);
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

						case SDL_SCANCODE_F1:
							log_info();
							break;
					}
					break;

				case SDL_WINDOWEVENT:
					switch (cur_event.window.event) {
						case SDL_WINDOWEVENT_SIZE_CHANGED:
							on_window_resize(cur_event.window.data1, cur_event.window.data2);
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