#version 330 core
// Port of windows/shaders.hlsl bloom_threshold (four taps, soft knee).
in vec2 uv;
uniform sampler2D source;
uniform vec2 texel;
out vec4 color;
void main() {
    vec3 c = (texture(source, uv + texel * vec2(-.5,-.5)).rgb
            + texture(source, uv + texel * vec2( .5,-.5)).rgb
            + texture(source, uv + texel * vec2(-.5, .5)).rgb
            + texture(source, uv + texel * vec2( .5, .5)).rgb) * .25;
    float lum = max(max(c.r,c.g),c.b);
    float k = smoothstep(.72 * .8 - .3, .72 * .8 + .3, lum);
    color = vec4(c * k * 1.5, 1);
}
