#pragma once

#include <glew/glew.h>
#include <glm/vec3.hpp>

namespace coral {

	class Model {
	public:

		struct Vertex {
			glm::vec3 position;
		};

	private:

		GLuint vao = NULL;
		GLuint vbo = NULL;
		unsigned int vertex_count;

	public:

		Model(const Vertex* vertices, unsigned int count, const char* label);
		~Model();

		Model(const Model&) = delete;
		Model& operator=(const Model&) = delete;

		Model(Model&&) = default;
		Model& operator=(Model&&) = default;

		void draw() const noexcept;

	};

} // namespace coral

