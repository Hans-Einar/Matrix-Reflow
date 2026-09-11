#version 330 core
in vec2 uv, local;
in vec3 glyphColor;
in float brightness, fog, shimmer;
uniform sampler2D atlas;
uniform float textured, wireframe, extraContrastHeads;
out vec4 color;
void main() {
    float coverage;
    if (wireframe > 0.5) {
        vec2 d = min(local, 1.0 - local);
        coverage = 1.0 - smoothstep(0.04, 0.06, min(d.x, d.y));
    } else coverage = textured > 0.5 ? texture(atlas, uv).r : 1.0;
    if (coverage < 0.01) discard;
    float b = pow(max(brightness, 0.0), 1.2) * fog * shimmer;
    vec3 base = glyphColor * b;
    float contrast = smoothstep(0.76, 0.80, brightness) * extraContrastHeads;
    float luminance = dot(base, vec3(0.299, 0.587, 0.114));
    base = mix(base, vec3(luminance), contrast);
    // Preserve upstream's hot term, even though core brightness currently caps at 0.8.
    vec3 whiteFlash = vec3(0.35, 0.35, 0.3) * smoothstep(0.95, 1.0, brightness);
    color = vec4((base + whiteFlash) * coverage * 1.5,
                 coverage * smoothstep(0.0, 0.15, b));
}
