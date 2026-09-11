#version 330 core
in vec2 uv;
uniform sampler2D scene;
out vec4 color;
void main() {
    // Final output is opaque regardless of scene/bloom alpha.
    color = vec4(texture(scene, uv).rgb, 1.0);
}
