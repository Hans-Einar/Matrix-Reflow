#version 330 core
in vec2 uv;
out vec4 color;
void main() { color = vec4(uv.x, uv.y, 0.25, 1.0); }
