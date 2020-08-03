#version 450 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec4 color;

out VertexData {
    vec4 color;
} OutData;

void main()
{
    gl_Position = vec4(position, 1.0);
    OutData.color = color;
}