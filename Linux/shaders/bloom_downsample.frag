#version 330 core
// Port of the reference 13-tap normalized downsampling kernel.
in vec2 uv;
uniform sampler2D source;
uniform vec2 texel;
out vec4 color;
vec3 tap(float x, float y) { return texture(source, uv + texel * vec2(x,y)).rgb; }
void main() {
    vec3 c = tap(0,0) * .125;
    c += (tap(-2,2)+tap(2,2)+tap(-2,-2)+tap(2,-2)) * .03125;
    c += (tap(0,2)+tap(-2,0)+tap(2,0)+tap(0,-2)) * .0625;
    c += (tap(-1,1)+tap(1,1)+tap(-1,-1)+tap(1,-1)) * .125;
    color = vec4(c,1);
}
