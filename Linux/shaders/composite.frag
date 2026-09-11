#version 330 core
in vec2 uv;
uniform sampler2D scene, bloom;
uniform bool postEnabled;
uniform float bloomIntensity, distortion;
out vec4 color;
void main() {
    if(!postEnabled) {color=vec4(texture(scene,uv).rgb,1);return;}
    // Reference SDR path: linear FP16 arithmetic, then UNORM output. No extra
    // gamma or tone mapping: the reference SDR target is not an sRGB view.
    vec2 center=uv-.5;
    float edge=dot(center,center);
    vec2 base=center*(1+distortion*edge)+.5;
    // Reflection-symmetric kernel/warp; GL's bottom-up UV needs no extra flip.
    vec2 off=normalize(center+vec2(1e-5))*distortion*.05*.5*edge;
    vec3 s=vec3(texture(scene,base+off).r,texture(scene,base).g,texture(scene,base-off).b);
    vec3 b=bloomIntensity>0 ? texture(bloom,base).rgb : vec3(0);
    float vignette=1-smoothstep(.3,1.0,length(center));
    vec2 margin=vec2(.006+distortion*.16)+abs(off);
    vec2 m=smoothstep(vec2(0),margin,base)*smoothstep(vec2(0),margin,1-base);
    color=vec4((s+b*bloomIntensity)*vignette*m.x*m.y,1);
}
