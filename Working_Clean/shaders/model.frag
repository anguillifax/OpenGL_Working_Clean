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
    vec3 color = vec3(0.95, 0.2, 0.0);
    OutColor = vec4(color, 1.0);
}