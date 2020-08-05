#version 450 core

const float PI = 3.14159;
const float TWO_PI = PI * 2;

layout (std140, binding = 0) uniform ApplicationBlock {
    ivec2 window_size;
    ivec2 mouse_position;
    float total_time;
    float corrected_time;
} Application;

out vec4 OutColor;

void main() 
{
    vec2 uv = gl_FragCoord.xy / Application.window_size;
    vec3 color = vec3(uv, 1.0);

    float line_pos = 0.5 * -cos(Application.corrected_time * TWO_PI / 4.0) + 0.5;
    float factor = 0.0;
    if (gl_FragCoord.x < line_pos * Application.window_size.x) {
        factor = 1.0;
    }

    color = mix(vec3(0.05), color, factor);

    if (distance(gl_FragCoord.xy, Application.mouse_position) < 20) {
        color = vec3(1.0);
    }

    OutColor = vec4(color, 1.0);
}