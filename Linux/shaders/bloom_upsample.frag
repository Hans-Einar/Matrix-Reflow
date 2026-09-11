#version 330 core
in vec2 uv;
uniform sampler2D source;
uniform vec2 texel;
out vec4 color;
vec3 tap(float x,float y) {return texture(source,uv+texel*vec2(x,y)).rgb;}
void main() {
    vec3 c=tap(0,0)*4;
    c+=(tap(0,1)+tap(-1,0)+tap(1,0)+tap(0,-1))*2;
    c+=tap(-1,1)+tap(1,1)+tap(-1,-1)+tap(1,-1);
    color=vec4(c/16,1);
}
