#version 450 core

layout (location = 0) in vec3 position;

layout (location = 0) uniform mat4 model_view_matrix;
layout (location = 1) uniform mat4 proj_matrix;

void main()
{
    gl_Position = proj_matrix * model_view_matrix * vec4(position, 1.0);
}