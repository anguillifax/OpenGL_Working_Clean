#include "Model.h"

#include <string>

namespace coral {

	struct AttributeIndex {
		enum {
			POSITION = 0,
		};
	};

	Model::Model(const Vertex* vertices, unsigned int size, const char* label)
		: vertex_count(size)
	{
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glObjectLabel(GL_VERTEX_ARRAY, vao, -1, label);

		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glObjectLabel(GL_BUFFER, vbo, -1, (std::string(label) + ".VBO").c_str());
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * size, vertices, GL_STATIC_DRAW);

		glEnableVertexAttribArray(AttributeIndex::POSITION);
		glVertexAttribPointer(AttributeIndex::POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));

		glBindBuffer(GL_ARRAY_BUFFER, NULL);

		glBindVertexArray(NULL);
	}

	Model::~Model()
	{
		glDeleteBuffers(1, &vbo);
		glDeleteVertexArrays(1, &vao);
	}

	void Model::draw() const noexcept
	{
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, vertex_count);
		glBindVertexArray(NULL);
	}

} // namespace coral
