#version 330 core
// Port of windows/shaders.hlsl::crt_filter. No temporal/history buffer.
in vec2 uv;
uniform sampler2D source;
uniform vec2 resolution;
uniform float time;
uniform bool identity;
out vec4 color;
void main() {
    if(identity) {color=vec4(texture(source,uv).rgb,1);return;}
    vec2 texel=1.0/resolution;
    // HLSL SV_Position is top-left, at pixel centers. Anchor the pattern to
    // output pixels (not warped UV), including odd-sized windows and captures.
    vec2 pixel=vec2(gl_FragCoord.x,resolution.y-gl_FragCoord.y);
    float phase=fract(pixel.x/2.2);
    vec3 mask=phase<1.0/3.0 ? vec3(1.08,.82,.82) :
              phase<2.0/3.0 ? vec3(.82,1.08,.82) : vec3(.82,.82,1.08);
    float scanPhase=fract(pixel.y/2.0);
    float scanline=.72+.28*smoothstep(0.0,.5,1-abs(scanPhase*2-1));
    scanline*=.985+.015*sin(pixel.y*1.7+time*.6);
    vec3 bleed=texture(source,uv-vec2(2*texel.x,0)).rgb*.06
              +texture(source,uv-vec2(texel.x,0)).rgb*.18
              +texture(source,uv).rgb*.52
              +texture(source,uv+vec2(texel.x,0)).rgb*.18
              +texture(source,uv+vec2(2*texel.x,0)).rgb*.06;
    vec3 converged=vec3(texture(source,uv+vec2(.6*texel.x,0)).r,
                        bleed.g,texture(source,uv-vec2(.6*texel.x,0)).b);
    vec3 c=mix(bleed,converged,.5);
    float lum=dot(c,vec3(.299,.587,.114));
    c*=mix(vec3(1),mask,.85*clamp(lum*2,0,1));
    c*=scanline;
    c=.02+c*(1-.02);
    c=c/(1+c*.15);
    vec2 center=uv-.5;
    c*=1-.22*smoothstep(.12,.5,dot(center,center));
    color=vec4(c,1);
}
