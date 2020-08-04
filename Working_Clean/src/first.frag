#version 450 core

const float PI = 3.14159;
const float TWO_PI = PI * 2;

in VertexDataBlock {
    vec4 color;
} VertexData;

layout (std140, binding = 0) uniform TimeBlock {
    float unscaled;
    float corrected;
} Time;

layout (location = 0) uniform ivec2 u_WindowSize;
layout (location = 1) uniform float u_Time;
layout (location = 2) uniform ivec2 u_MousePosition;

out vec4 OutColor;

void main() 
{
    vec2 uv = gl_FragCoord.xy / u_WindowSize.xy;
    vec3 color = vec3(uv, 1.0);

    float line_pos = 0.5 * sin(Time.corrected * TWO_PI / 4.0) + 0.5;
    line_pos = 0.5;
    float factor = 0.0;
    if (uv.x > line_pos * u_WindowSize.x) {
        factor = 1.0;
    }

    color = mix(vec3(0.1), color, factor);

    OutColor = vec4(color, 1.0);
}