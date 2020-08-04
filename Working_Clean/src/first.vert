#version 450 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec4 color;

out VertexDataBlock {
    vec4 color;
} VertexData;

layout (std140) uniform TransformBlock {
    vec2 scale;
    float rotation; // ccw degrees
    vec3 translation;
    mat4 projection_matrix;
} Transform;


void main()
{
    gl_Position = vec4(position, 1.0);
    VertexData.color = color;
}