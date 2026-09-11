#version 330 core
// Initial identity pass; the reference CRT treatment follows in I4-M2.
in vec2 uv;
uniform sampler2D source;
out vec4 color;
void main() { color=vec4(texture(source,uv).rgb,1); }
