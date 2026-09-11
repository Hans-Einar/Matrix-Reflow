#version 330 core
layout(location=0) in vec4 positionCell;
layout(location=1) in vec3 flipBrightness;
layout(location=2) in vec3 instanceColor;
uniform mat4 viewProjection;
uniform vec3 cameraRight, cameraUp, cameraPosition;
uniform float glyphHalf, time;
uniform vec2 atlasGrid;
uniform vec3 fogParameters; // enabled, start distance, end distance
out vec2 uv, local;
out vec3 glyphColor;
out float brightness, fog, shimmer;
void main() {
    vec2 corner = vec2((gl_VertexID == 1 || gl_VertexID == 3) ? 1.0 : -1.0,
                       gl_VertexID >= 2 ? 1.0 : -1.0);
    vec3 world = positionCell.xyz + cameraRight * corner.x * glyphHalf
                                  + cameraUp * corner.y * glyphHalf;
    gl_Position = viewProjection * vec4(world, 1.0);
    vec2 flipped = corner * flipBrightness.xy;
    local = vec2(flipped.x * 0.5 + 0.5, 0.5 - flipped.y * 0.5);
    uv = (vec2(mod(positionCell.w, atlasGrid.x), floor(positionCell.w / atlasGrid.x)) + local) / atlasGrid;
    float distanceToCamera = length(positionCell.xyz - cameraPosition);
    fog = fogParameters.x > 0.5 ?
        1.0 - smoothstep(fogParameters.y, fogParameters.z, distanceToCamera) : 1.0;
    glyphColor = instanceColor;
    brightness = flipBrightness.z;
    shimmer = 0.9 + 0.1 * sin(time * 5.0 + positionCell.y * 0.1);
}
