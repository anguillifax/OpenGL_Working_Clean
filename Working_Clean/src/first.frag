#version 450 core

const float PI = 3.14159;

in VertexData {
    vec4 color;
} InData;

out vec4 OutColor;

layout (location = 0) uniform ivec2 u_WINDOW_SIZE;
layout (location = 1) uniform float u_TIME;
layout (location = 2) uniform ivec2 u_MOUSE_POSITION;

void main() 
{
    // vec3 color = gl_FragCoord.xyz;

    // color.x /= u_WINDOW_SIZE.x;
    // color.y /= u_WINDOW_SIZE.y;

    // color.b = 0.2 * sin(u_TIME * 2 * PI / 4) + 0.8;
    // color.b = 1.0;

    // float dist = distance(gl_FragCoord.xy, u_MOUSE_POSITION);
    // if (dist < 30) {
    //     color = vec3(1);
    // }

    OutColor = vec4(InData.color.xyz, 1.0);
}